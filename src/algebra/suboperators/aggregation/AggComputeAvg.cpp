#include "algebra/suboperators/aggregation/AggComputeAvg.h"
#include "codegen/Expression.h"
#include "codegen/IRBuilder.h"
#include "codegen/Statement.h"

namespace inkfuse {

AggComputeAvg::AggComputeAvg(IR::TypeArc type_)
   : AggCompute(std::move(type_)) {
}

IR::StmtPtr AggComputeAvg::compute(IR::FunctionBuilder& builder, const std::vector<IR::Stmt*>& granule_ptrs, const IR::Stmt& out_val) const {
   assert(granule_ptrs.size() == 2);
   const auto& sum = *granule_ptrs[0];
   const auto& count = *granule_ptrs[1];
   // Extract the sum.
   auto expr_sum_ptr = IR::CastExpr::build(IR::VarRefExpr::build(sum), IR::Pointer::build(type));
   // Cast to double.
   auto expr_sum_casted = IR::CastExpr::build(IR::DerefExpr::build(std::move(expr_sum_ptr)), IR::Float::build(8));
   // Extract the count.
   auto expr_count_ptr = IR::DerefExpr::build(IR::CastExpr::build(IR::VarRefExpr::build(count), IR::Pointer::build(IR::UnsignedInt::build(8))));
   // Divide and assign.
   auto result = IR::ArithmeticExpr::build(std::move(expr_sum_casted), std::move(expr_count_ptr), IR::ArithmeticExpr::Opcode::Divide);
   return IR::AssignmentStmt::build(IR::VarRefExpr::build(out_val), std::move(result));
}

std::string AggComputeAvg::id() const {
   return "agg_compute_avg_" + type->id();
}

}