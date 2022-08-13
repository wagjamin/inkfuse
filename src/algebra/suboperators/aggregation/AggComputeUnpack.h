#ifndef INKFUSE_AGGCOMPUTEUNPACK_H
#define INKFUSE_AGGCOMPUTEUNPACK_H

#include "algebra/suboperators/aggregation/AggCompute.h"

namespace inkfuse {

struct AggComputeUnpack : public AggCompute {
   AggComputeUnpack(IR::TypeArc type_);

   IR::StmtPtr compute(IR::FunctionBuilder& builder, const std::vector<IR::Stmt*>& granule_ptrs, const IR::Stmt& out_val) const override;

   std::string id() const override;
};

}

#endif //INKFUSE_AGGCOMPUTEUNPACK_H
