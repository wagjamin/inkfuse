#ifndef INKFUSE_AGGREADERSUBOP_H
#define INKFUSE_AGGREADERSUBOP_H

#include "algebra/suboperators/Suboperator.h"
#include "algebra/suboperators/aggregation/AggCompute.h"
#include "algebra/suboperators/properties/WithRuntimeParams.h"
#include "algebra/suboperators/row_layout/KeyPackingRuntimeState.h"

namespace inkfuse {

/// AggReaderSubop computes an aggregate function result based on provided aggregate state.
/// Binary runtime state as some agg functions such as avg require two offsets (sum/count).
struct AggReaderSubop : public TemplatedSuboperator<KeyPackingRuntimeStateTwo>, public WithRuntimeParams<KeyPackingRuntimeParamsTwo> {
   static SuboperatorArc build(const RelAlgOp* source_, const IU& packed_ptr_iu, const IU& agg_iu, const AggCompute& agg_compute_);

   void setUpStateImpl(const ExecutionContext& context) override;

   void consumeAllChildren(CompilationContext& context) override;

   std::string id() const override;

   private:
   AggReaderSubop(const RelAlgOp* source_, const IU& packed_ptr_iu, const IU& agg_iu, const AggCompute& agg_compute_);

   /// Backing compute logic for the result aggregate.
   const AggCompute& agg_compute;
};

}

#endif //INKFUSE_AGGREADERSUBOP_H
