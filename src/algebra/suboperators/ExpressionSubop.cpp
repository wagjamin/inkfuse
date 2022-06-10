#include "algebra/suboperators/ExpressionSubop.h"
#include "algebra/CompilationContext.h"
#include "codegen/Statement.h"
#include <sstream>

namespace inkfuse {

namespace {

using Type = ExpressionOp::ComputeNode::Type;

/// Actual names of the operations for id generation.
const std::unordered_map<Type, std::string> expr_names{
   {Type::Add, "add"},
   {Type::Cast, "cast"},
   {Type::Subtract, "sub"},
   {Type::Multiply, "mul"},
   {Type::Divide, "div"},
   {Type::Greater, "gt"},
   {Type::GreaterEqual, "ge"},
   {Type::Less, "lt"},
   {Type::LessEqual, "le"},
   {Type::Eq, "eq"}};

/// Map from algebra expression types to IR expressions in the jitted code.
const std::unordered_map<Type, IR::ArithmeticExpr::Opcode> code_map{
   {Type::Add, IR::ArithmeticExpr::Opcode::Add},
   {Type::Subtract, IR::ArithmeticExpr::Opcode::Subtract},
   {Type::Multiply, IR::ArithmeticExpr::Opcode::Multiply},
   {Type::Divide, IR::ArithmeticExpr::Opcode::Divide},
   {Type::Greater, IR::ArithmeticExpr::Opcode::Greater},
   {Type::GreaterEqual, IR::ArithmeticExpr::Opcode::GreaterEqual},
   {Type::Less, IR::ArithmeticExpr::Opcode::Less},
   {Type::LessEqual, IR::ArithmeticExpr::Opcode::LessEqual},
   {Type::Eq, IR::ArithmeticExpr::Opcode::Eq},
};

}

// static
SuboperatorArc ExpressionSubop::build(const RelAlgOp* source_, std::vector<const IU*> provided_ius_, std::vector<const IU*> operands_, ExpressionOp::ComputeNode::Type type_) {
   return std::make_shared<ExpressionSubop>(source_, std::move(provided_ius_), std::move(operands_), type_);
}

ExpressionSubop::ExpressionSubop(const RelAlgOp* source_, std::vector<const IU*> provided_ius_, std::vector<const IU*> source_ius_, ExpressionOp::ComputeNode::Type type_)
   : TemplatedSuboperator<EmptyState, EmptyState>(source_, std::move(provided_ius_), std::move(source_ius_)), type(type_) {
}

void ExpressionSubop::consumeAllChildren(CompilationContext& context) {
   // All children were produced, we simply have to generate code for this specific expression.
   auto& builder = context.getFctBuilder();
   const auto& program = context.getProgram();
   const IU* out = *provided_ius.begin();

   // First, declare the output IU.
   auto iu_name = context.buildIUIdentifier(*out);
   auto& declare = builder.appendStmt(IR::DeclareStmt::build(iu_name, out->type));
   // Declare the output IU.
   context.declareIU(*out, declare);

   // Then, compute it. First resolve the child IU statements.
   std::vector<const IR::Stmt*> children;
   children.reserve(source_ius.size());
   for (auto elem : source_ius) {
      const auto& decl = context.getIUDeclaration(*elem);
      children.push_back(&decl);
   }
   if (type == Type::Cast) {
      assert(children.size() == 1);
      // Cast Op.
      builder.appendStmt(
         IR::AssignmentStmt::build(
            declare,
            IR::CastExpr::build(
               IR::VarRefExpr::build(*children[0]),
               out->type)));
   } else {
      // Binary Arithmetic Op.
      assert(children.size() == 2);
      builder.appendStmt(
         IR::AssignmentStmt::build(
            declare,
            IR::ArithmeticExpr::build(
               IR::VarRefExpr::build(*children[0]),
               IR::VarRefExpr::build(*children[1]),
               code_map.at(type))));
   }
   context.notifyIUsReady(*this);
}

std::string ExpressionSubop::id() const {
   std::stringstream res;
   res << "expr_" << expr_names.at(type);
   for (auto child : source_ius) {
      res << "_" << child->type->id();
   }
   res << "_";
   for (auto out : provided_ius) {
      res << "_" << out->type->id();
   }
   return res.str();
}

}
