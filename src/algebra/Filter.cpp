#include "algebra/Filter.h"
#include "algebra/Pipeline.h"
#include "algebra/suboperators/FilterSubop.h"

namespace inkfuse {

Filter::Filter(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string name, const IU& filter_iu_)
   : RelAlgOp(std::move(children_), std::move(name)), filter_iu(filter_iu_)
{
}

void Filter::decay(std::unordered_set<const IU*> required, PipelineDAG& dag) const
{
   // Our children need to produce everything required upstream, plus the filter IU.
   auto downstream_required = required;
   downstream_required.insert(&filter_iu);
   // First decay the children.
   assert(children.size() == 1);
   children[0]->decay(std::move(downstream_required), dag);
   // Attach the filter sub-operator. Note that it rescopes the pipeline.
   auto& pipe = dag.getCurrentPipeline();
   // But the actual filter only needs to produce the actually upstream required IUs.
   auto subop = std::make_shared<FilterSubop>(this, std::move(required), filter_iu);
   pipe.attachSuboperator(std::move(subop));
}

void Filter::addIUs(std::unordered_set<const IU*>& set) const
{
   // NOOP as the filter does not create new IUs.
}

}