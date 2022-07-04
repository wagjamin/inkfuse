#include "algebra/suboperators/expressions/LazyExpressionSubop.h"
#include "algebra/suboperators/expressions/ExpressionHelpers.h"
#include "codegen/IRBuilder.h"
#include "runtime/Runtime.h"

namespace inkfuse {

using Type = ExpressionOp::ComputeNode::Type;

const char* LazyExpressionSubopState::name = "LazyExpressionSubopState";

void LazyExpressionSubop::registerRuntime() {
   RuntimeStructBuilder{LazyExpressionSubopState::name}
      .addMember("data_erased", IR::Pointer::build(IR::Void::build()));
}

SuboperatorArc LazyExpressionSubop::build(const RelAlgOp* source_, std::vector<const IU*> provided_ius_, std::vector<const IU*> operands_, ExpressionOp::ComputeNode::Type type_, IR::TypeArc lazy_expression_type_) {
   return std::make_shared<LazyExpressionSubop>(source_, std::move(provided_ius_), std::move(operands_), type_, std::move(lazy_expression_type_));
}

LazyExpressionSubop::LazyExpressionSubop(const RelAlgOp* source_, std::vector<const IU*> provided_ius_, std::vector<const IU*> source_ius_, ExpressionOp::ComputeNode::Type type_, IR::TypeArc lazy_expression_type_)
   : TemplatedSuboperator<LazyExpressionSubopState, EmptyState>(source_, std::move(provided_ius_), std::move(source_ius_)), lazy_param_type(std::move(lazy_expression_type_)), type(type_) {
   if (type == Type::Hash || type == Type::Cast) {
      throw std::runtime_error("LazyExpressionSubop only supports binary operands.");
   }
}

void LazyExpressionSubop::setUpStateImpl(const ExecutionContext& context) {
   // Lazy params have to be set up during execution and have the right type.
   if (!lazy_params.data || lazy_param_type->id() != lazy_params.data->getType()->id()) {
      throw std::runtime_error("LazyParam must be set up and of the right type during execution.");
   }
   // Fetch the underlying raw data from the associated lazy parameters.
   // If the value was hard-coded in the generated code already it will simply never be accessed.
   state->data_erased = lazy_params.data->rawData();
}

void LazyExpressionSubop::consumeAllChildren(CompilationContext& context) {
   // All children were produced, we simply have to generate code for this specific expression.
   auto& builder = context.getFctBuilder();
   const auto& program = context.getProgram();
   const IU* out = *provided_ius.begin();

   // First, declare the output IU.
   auto iu_name = context.buildIUIdentifier(*out);
   auto& declare = builder.appendStmt(IR::DeclareStmt::build(iu_name, out->type));
   context.declareIU(*out, declare);

   // Then, compute it. Lazily resolve the second argument.
   const IR::Stmt& child = context.getIUDeclaration(*source_ius[0]);
   builder.appendStmt(
      IR::AssignmentStmt::build(
         declare,
         IR::ArithmeticExpr::build(
            IR::VarRefExpr::build(child),
            lazy_params.dataResolveErased(*this, lazy_param_type, context),
            ExpressionHelpers::code_map.at(type))));

   context.notifyIUsReady(*this);
}

std::string LazyExpressionSubop::id() const {
   std::stringstream res;
   res << "lazyexpr_" << ExpressionHelpers::expr_names.at(type);
   for (auto child : source_ius) {
      res << "_" << child->type->id();
   }
   res << "_" << lazy_param_type->id();
   res << "_";
   for (auto out : provided_ius) {
      res << "_" << out->type->id();
   }
   return res.str();
}
}
