#include "algebra/suboperators/aggregation/AggStateCount.h"
#include "algebra/CompilationContext.h"
#include "codegen/Statement.h"

namespace inkfuse {

AggStateCount::AggStateCount()
   : ZeroInitializedAggState(std::move(IR::Void::build())) {
}

void AggStateCount::updateState(IR::FunctionBuilder& builder, const IR::Stmt& ptr, const IR::Stmt&) const {
   // Increment count by one.
   auto casted_ptr_expr_curr = IR::CastExpr::build(IR::VarRefExpr::build(ptr), IR::Pointer::build(IR::SignedInt::build(8)));
   auto casted_ptr_expr_assign = IR::CastExpr::build(IR::VarRefExpr::build(ptr), IR::Pointer::build(IR::SignedInt::build(8)));
   // Get new counter.
   auto new_val = IR::ArithmeticExpr::build(
      IR::ConstExpr::build(IR::SI<8>::build(1)), IR::DerefExpr::build(std::move(casted_ptr_expr_curr)), IR::ArithmeticExpr::Opcode::Add);
   // And assign.
   builder.appendStmt(IR::AssignmentStmt::build(IR::DerefExpr::build(std::move(casted_ptr_expr_assign)), std::move(new_val)));
}

size_t AggStateCount::getStateSize() const {
   return 8;
}

std::string AggStateCount::id() const {
   return "agg_state_count";
}

}