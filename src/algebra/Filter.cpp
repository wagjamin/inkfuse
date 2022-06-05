#include "algebra/Filter.h"
#include "algebra/Pipeline.h"
#include "algebra/suboperators/FilterSubop.h"

namespace inkfuse {

Filter::Filter(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string name, const IU& filter_iu_)
   : RelAlgOp(std::move(children_), std::move(name)), filter_iu(filter_iu_) {
}

void Filter::decay(std::unordered_set<const IU*> required, PipelineDAG& dag) const {
   // Our children need to produce everything required upstream, plus the filter IU.
   required.insert(&filter_iu);
   // First decay the children.
   assert(children.size() == 1);
   children[0]->decay(required, dag);
   // Attach the filter sub-operator. Note that it rescopes the pipeline.
   auto& pipe = dag.getCurrentPipeline();
   auto subop = std::make_shared<FilterSubop>(this, std::move(required), filter_iu);
   pipe.attachSuboperator(std::move(subop));
}

void Filter::addIUs(std::unordered_set<const IU*>& set) const {
   // NOOP as the filter does not create new IUs.
}

}