#ifndef INKFUSE_AGGREADERSUBOP_H
#define INKFUSE_AGGREADERSUBOP_H

#include "algebra/suboperators/Suboperator.h"
#include "algebra/suboperators/aggregation/AggCompute.h"
#include "algebra/suboperators/properties/WithRuntimeParams.h"
#include "algebra/suboperators/row_layout/KeyPackingRuntimeState.h"

namespace inkfuse {

/// AggCompute classes can go over different runtime parameters. They are a bit more
/// complex than the AggState which always goes over a single pointer offset.
/// Take average for example. Avg needs to be able to find the sum and the count state.
template <typename AggFct>
concept IsAggCompute = std::is_base_of<AggCompute, AggFct>::value && requires(T t) {
   { t.attachRuntimeParams(AggFct::RuntimeParams) } -> void;
};

/// AggReaderSubop computes an aggregate function result based on provided aggregate state
/// .
template <IsAggCompute AggFct>
struct AggReaderSubop : public TemplatedSuboperator<KeyPackingRuntimeState>, public WithRuntimeParams<AggFct::RuntimeParams> {
   static SuboperatorArc build(const RelAlgOp* source_, const IU& agg_iu);

   void setUpStateImpl(const ExecutionContext& context) override;

   void consumeAllChildren(CompilationContext& context) override;

   bool isSource() const override { return true; };

   std::string id() const override;

   private:
   AggReaderSubop(const RelAlgOp* source_, const IU& out_iu, const IU& ptr_iu);
};

}

#endif //INKFUSE_AGGREADERSUBOP_H
