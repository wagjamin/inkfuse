#include "algebra/suboperators/aggregation/AggregatorSubop.h"
#include "algebra/suboperators/aggregation/AggStateCount.h"

namespace inkfuse {

SuboperatorArc AggregatorSubop::build(const RelAlgOp* source_, const AggState& agg_state_, const IU& ptr_iu, const IU& is_init, const IU& agg_iu)
{

}

AggregatorSubop::AggregatorSubop(const RelAlgOp* source_, const AggState& agg_state_, const IU& ptr_iu, const IU& is_init, const IU& agg_iu)
   : TemplatedSuboperator<KeyPackingRuntimeState>(source_, {}, {&ptr_iu, &is_init, &agg_iu}), agg_state(agg_state_)
{
}

void AggregatorSubop::setUpStateImpl(const ExecutionContext& context)
{

}

void AggregatorSubop::consumeAllChildren(CompilationContext& context)
{
   // Compute the pointer to the aggregate state.

   // Is this the initial time we write to this group?

   // Call the group initialization logic.

   // Call the group update logic.

}


}
