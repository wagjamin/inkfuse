#ifndef INKFUSE_AGGSTATESUM_H
#define INKFUSE_AGGSTATESUM_H

#include "algebra/suboperators/aggregation/AggState.h"

namespace inkfuse {

/// Sum state for aggregates, same return type as the type being
/// aggregated. Starts out with zero-initialized memory.
struct AggStateSum : public ZeroInitializedAggState {

   AggStateSum(IR::TypeArc type_);

   void updateState(IR::FunctionBuilder& builder, const IR::Stmt& ptr, const IR::Stmt& val) const override;

   size_t getStateSize() const override;

   std::string id() const override;
};

}

#endif //INKFUSE_AGGSTATESUM_H
