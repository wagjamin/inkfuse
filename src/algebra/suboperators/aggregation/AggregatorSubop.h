#ifndef INKFUSE_AGGREGATORSUBOP_H
#define INKFUSE_AGGREGATORSUBOP_H

#include "algebra/suboperators/Suboperator.h"
#include "algebra/suboperators/aggregation/AggState.h"
#include "algebra/suboperators/properties/WithRuntimeParams.h"
#include "algebra/suboperators/row_layout/KeyPackingRuntimeState.h"

namespace inkfuse {

/// AggregatorSubop computes a single aggregate state within a compound aggregate state.
/// It takes a key offset as runtime parameter indicating where the aggregate state is
/// located in the overall compound payload.
/// In InkFuse, the AggregatorSubop operates on a specific aggregate state, not an aggregate
/// function. The computed aggregate functions can share state. For example if we calculated
/// 'SELECT sum(x), avg(x), count(x) FROM T', the packed aggregate key would only contain [StateCount, StateSum].
struct AggregatorSubop : public TemplatedSuboperator<KeyPackingRuntimeState>, public WithRuntimeParams<KeyPackingRuntimeParams> {
   static SuboperatorArc build(const RelAlgOp* source_, const AggState& agg_state, const IU& ptr_iu, const IU& not_init, const IU& agg_iu);

   void setUpStateImpl(const ExecutionContext& context) override;

   void consumeAllChildren(CompilationContext& context) override;

   bool isSink() const override { return true; };

   std::string id() const override;

   protected:
   AggregatorSubop(const RelAlgOp* source_, const AggState& agg_state, const IU& ptr_iu, const IU& not_init, const IU& agg_iu);

   /// Backing aggregate granule containing the state update logic.
   const AggState& agg_state;
};

}

#endif //INKFUSE_AGGREGATORSUBOP_H
