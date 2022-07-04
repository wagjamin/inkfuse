#include "algebra/suboperators/expressions/ExpressionSubop.h"
#include "algebra/suboperators/expressions/ExpressionHelpers.h"
#include "algebra/CompilationContext.h"
#include "codegen/Statement.h"
#include <sstream>

namespace inkfuse {

using Type = ExpressionOp::ComputeNode::Type;

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
   } else if (type == Type::Hash) {
      assert(children.size() == 1);
      // Hash Op. Call into hashing runtime.
      buildHashingExpression(context, builder, declare);
   } else {
      // Binary Arithmetic Op.
      assert(children.size() == 2);
      builder.appendStmt(
         IR::AssignmentStmt::build(
            declare,
            IR::ArithmeticExpr::build(
               IR::VarRefExpr::build(*children[0]),
               IR::VarRefExpr::build(*children[1]),
               ExpressionHelpers::code_map.at(type))));
   }
   context.notifyIUsReady(*this);
}

std::string ExpressionSubop::id() const {
   std::stringstream res;
   res << "expr_" << ExpressionHelpers::expr_names.at(type);
   for (auto child : source_ius) {
      res << "_" << child->type->id();
   }
   res << "_";
   for (auto out : provided_ius) {
      res << "_" << out->type->id();
   }
   return res.str();
}

void ExpressionSubop::buildHashingExpression(CompilationContext& context, IR::FunctionBuilder& builder, const IR::Stmt& declare) const {
   const IR::Function* hash_fct;
   std::vector<IR::ExprPtr> args;
   // First argument is a pointer to the data.
   args.push_back(IR::RefExpr::build(IR::VarRefExpr::build(context.getIUDeclaration(*source_ius[0]))));
   const IU& iu = *source_ius[0];
   // Very simple hashing logic for now - this needs to be updated once we have string support.
   if (iu.type->numBytes() == 4) {
      // Call into specialized 4 byte runtime.
      hash_fct = context.getRuntimeFunction("hash4").get();
   } else if (iu.type->numBytes() == 8) {
      // Call into specialized 8 byte runtime.
      hash_fct = context.getRuntimeFunction("hash8").get();
   } else {
      // Call into general purpose runtime.
      hash_fct = context.getRuntimeFunction("hash").get();
      // This one takes the size as a second argument.
      args.push_back(IR::ConstExpr::build(IR::UI<8>::build(iu.type->numBytes())));
   }
   IR::ExprPtr hashed_expr =
      IR::InvokeFctExpr::build(*hash_fct, std::move(args));
   // And build the assignment into the output IU.
   builder.appendStmt(
      IR::AssignmentStmt::build(
         declare, std::move(hashed_expr)));
}

}
