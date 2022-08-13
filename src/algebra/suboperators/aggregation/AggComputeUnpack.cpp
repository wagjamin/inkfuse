#include "algebra/suboperators/aggregation/AggComputeUnpack.h"
#include "algebra/suboperators/row_layout/KeyUnpackerSubop.h"

namespace inkfuse {

AggComputeUnpack::AggComputeUnpack(IR::TypeArc type_)
   : AggCompute(std::move(type_)) {
}

IR::StmtPtr AggComputeUnpack::compute(IR::FunctionBuilder& builder, const std::vector<IR::Stmt*>& granule_ptrs, const IR::Stmt& out_val) const {
   assert(granule_ptrs.size() == 1);
   // Offset 0 as we already get the pointer to the granule.
   return KeyPacking::unpackFrom(IR::VarRefExpr::build(*granule_ptrs[0]), IR::VarRefExpr::build(out_val), IR::ConstExpr::build(IR::UI<2>::build(0)));
}

std::string AggComputeUnpack::id() const
{
   return "agg_compute_unpack_" + type->id();
}

}