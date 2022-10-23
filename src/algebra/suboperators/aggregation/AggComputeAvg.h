#ifndef INKFUSE_AGGCOMPUTEAVG_H
#define INKFUSE_AGGCOMPUTEAVG_H

#include "algebra/suboperators/aggregation/AggCompute.h"

namespace inkfuse {

/// Compute an average aggregation result. Takes two granules:
/// first the sum over the target type and then an 8 byte count.
struct AggComputeAvg : public AggCompute {
   AggComputeAvg(IR::TypeArc type_);

   IR::StmtPtr compute(IR::FunctionBuilder& builder, const std::vector<IR::Stmt*>& granule_ptrs, const IR::Stmt& out_val) const override;

   size_t requiredGranules() const override { return 2; }

   std::string id() const override;
};

}

#endif //INKFUSE_AGGCOMPUTEAVG_H
