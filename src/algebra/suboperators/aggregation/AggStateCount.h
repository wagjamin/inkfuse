#ifndef INKFUSE_AGGSTATECOUNT_H
#define INKFUSE_AGGSTATECOUNT_H

#include "algebra/suboperators/aggregation/AggState.h"

namespace inkfuse {

/// Counting state for aggregates, computes an 8 btye count.
struct AggStateCount : public AggState {

   AggStateCount();

   void initState(IR::FunctionBuilder& builder, IR::ExprPtr ptr, IR::ExprPtr val) const override;

   void updateState(IR::FunctionBuilder& builder, IR::ExprPtr ptr, IR::ExprPtr val) const override;

   size_t getStateSize() const override;

   std::string id() const override;

};

}

#endif //INKFUSE_AGGSTATECOUNT_H
