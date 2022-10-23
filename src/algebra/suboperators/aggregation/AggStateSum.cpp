#include "algebra/suboperators/aggregation/AggStateSum.h"
#include "algebra/CompilationContext.h"
#include "codegen/Statement.h"

namespace inkfuse {

AggStateSum::AggStateSum(IR::TypeArc type_)
   : ZeroInitializedAggState(std::move(type_)) {
}

void AggStateSum::updateState(IR::FunctionBuilder& builder, const IR::Stmt& ptr, const IR::Stmt& val) const {
   // Fetch the current sum state.
   auto casted_ptr_expr_curr = IR::CastExpr::build(IR::VarRefExpr::build(ptr), IR::Pointer::build(type));
   auto casted_ptr_expr_assign = IR::CastExpr::build(IR::VarRefExpr::build(ptr), IR::Pointer::build(type));
   // Get the thing we want to add.
   auto val_expr = IR::VarRefExpr::build(val);
   // Add them up.
   auto new_val = IR::ArithmeticExpr::build(
      std::move(val_expr), IR::DerefExpr::build(std::move(casted_ptr_expr_curr)), IR::ArithmeticExpr::Opcode::Add);
   // And assign.
   builder.appendStmt(IR::AssignmentStmt::build(IR::DerefExpr::build(std::move(casted_ptr_expr_assign)), std::move(new_val)));
}

size_t AggStateSum::getStateSize() const {
   // Aggregate state of sum matches the type being summed.
   return type->numBytes();
}

std::string AggStateSum::id() const {
   return "agg_state_sum_" + type->id();
}

}
