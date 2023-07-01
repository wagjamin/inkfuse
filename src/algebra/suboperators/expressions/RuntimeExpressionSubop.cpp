#include "algebra/suboperators/expressions/RuntimeExpressionSubop.h"
#include "algebra/suboperators/expressions/ExpressionHelpers.h"
#include "codegen/IRBuilder.h"
#include "runtime/Runtime.h"

namespace inkfuse {

using Type = ExpressionOp::ComputeNode::Type;

const char* RuntimeExpressionState::name = "RuntimeExpressionState";

void RuntimeExpressionSubop::registerRuntime() {
   RuntimeStructBuilder{RuntimeExpressionState::name}
      .addMember("data_erased", IR::Pointer::build(IR::Void::build()));
}

SuboperatorArc RuntimeExpressionSubop::build(const RelAlgOp* source_, std::vector<const IU*> provided_ius_, std::vector<const IU*> operands_, ExpressionOp::ComputeNode::Type type_, IR::TypeArc runtime_expression_type_) {
   return std::make_shared<RuntimeExpressionSubop>(source_, std::move(provided_ius_), std::move(operands_), type_, std::move(runtime_expression_type_));
}

RuntimeExpressionSubop::RuntimeExpressionSubop(const RelAlgOp* source_, std::vector<const IU*> provided_ius_, std::vector<const IU*> source_ius_, ExpressionOp::ComputeNode::Type type_, IR::TypeArc runtime_expression_type_)
   : TemplatedSuboperator<RuntimeExpressionState>(source_, std::move(provided_ius_), std::move(source_ius_)), runtime_param_type(std::move(runtime_expression_type_)), type(type_) {
   if (type == Type::Hash || type == Type::Cast) {
      throw std::runtime_error("RuntimeExpressionSubop only supports binary operands.");
   }
}

void RuntimeExpressionSubop::setUpStateImpl(const ExecutionContext& context) {
   // Runtime params have to be set up during execution and have the right type.
   if (!runtime_params.data || runtime_param_type->id() != runtime_params.data->getType()->id()) {
      throw std::runtime_error("RuntimeParam of RuntimeExpressionSubop must be set up and of the right type during execution.");
   }
   for (auto& state : *states) {
      // Fetch the underlying raw data from the associated runtime parameters.
      // If the value was hard-coded in the generated code already it will simply never be accessed.
      state.data_erased = runtime_params.data->rawData();
   }
}

void RuntimeExpressionSubop::consumeAllChildren(CompilationContext& context) {
   // All children were produced, we simply have to generate code for this specific expression.
   auto& builder = context.getFctBuilder();
   const auto& program = context.getProgram();
   const IU* out = *provided_ius.begin();

   // First, declare the output IU.
   auto iu_name = context.buildIUIdentifier(*out);
   auto& declare = builder.appendStmt(IR::DeclareStmt::build(iu_name, out->type));
   context.declareIU(*out, declare);

   // Resolve the runtime parameter once in the opening scope of the program.
   IR::Stmt* rt_c_declare;
   {
      auto& root = builder.getRootBlock();
      auto rt_constant_name = getVarIdentifier();
      rt_constant_name << "_rt_constant";
      auto rt_const_declare = IR::DeclareStmt::build(rt_constant_name.str(), runtime_param_type);
      auto rt_const_load = IR::AssignmentStmt::build(IR::VarRefExpr::build(*rt_const_declare), runtime_params.dataResolveErased(*this, runtime_param_type, context));
      rt_c_declare = rt_const_declare.get();
      root.appendStmt(std::move(rt_const_declare));
      root.appendStmt(std::move(rt_const_load));
   }

   // Then, compute the expression in the current scope based on the loaded constant.
   const IR::Stmt& child = context.getIUDeclaration(*source_ius[0]);
   builder.appendStmt(
      IR::AssignmentStmt::build(
         declare,
         IR::ArithmeticExpr::build(
            IR::VarRefExpr::build(*rt_c_declare),
            IR::VarRefExpr::build(child),
            ExpressionHelpers::code_map.at(type))));

   context.notifyIUsReady(*this);
}

std::string RuntimeExpressionSubop::id() const {
   std::stringstream res;
   res << "runtime_expr_" << ExpressionHelpers::expr_names.at(type);
   for (auto child : source_ius) {
      res << "_" << child->type->id();
   }
   res << "_" << runtime_param_type->id();
   res << "_";
   for (auto out : provided_ius) {
      res << "_" << out->type->id();
   }
   return res.str();
}
}
