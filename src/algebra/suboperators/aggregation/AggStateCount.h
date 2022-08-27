#ifndef INKFUSE_AGGSTATECOUNT_H
#define INKFUSE_AGGSTATECOUNT_H

#include "algebra/suboperators/aggregation/AggState.h"

namespace inkfuse {

/// Counting state for aggregates, computes an 8 byte count.
/// Starts out with zero-initialized memory.
struct AggStateCount : public ZeroInitializedAggState {
   AggStateCount();

   void updateState(IR::FunctionBuilder& builder, const IR::Stmt& ptr, const IR::Stmt& val) const override;

   size_t getStateSize() const override;

   std::string id() const override;
};

}

#endif //INKFUSE_AGGSTATECOUNT_H
