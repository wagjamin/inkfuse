#include "algebra/Filter.h"
#include "algebra/Pipeline.h"
#include "algebra/suboperators/ColumnFilter.h"

namespace inkfuse {

std::unique_ptr<Filter> Filter::build(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string name, std::vector<const IU*> redefined_, const IU& filter_iu_) {
   return std::unique_ptr<Filter>(new Filter(std::move(children_), std::move(name), std::move(redefined_), filter_iu_));
}

Filter::Filter(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string name, std::vector<const IU*> to_redefine_, const IU& filter_iu_)
   : RelAlgOp(std::move(children_), std::move(name)), filter_iu(filter_iu_), pseudo_iu(IR::Void::build()), to_redefine(std::move(to_redefine_)) {
   {
      redefined.reserve(to_redefine.size());
      output_ius.reserve(to_redefine.size());
      // Define the output IUs which we will use.
      for (const IU* iu : to_redefine) {
         redefined.emplace_back(iu->type);
      }
      // Set up output structure.
      for (const IU& iu : redefined) {
         output_ius.push_back(&iu);
      }
   }
}

void Filter::decay(PipelineDAG& dag) const
{
   // First decay the children.
   assert(children.size() == 1);
   children[0]->decay(dag);
   auto& pipe = dag.getCurrentPipeline();
   // Attach the control flow sub-operator.
   auto scope = ColumnFilterScope::build(this, to_redefine, filter_iu, pseudo_iu);
   pipe.attachSuboperator(std::move(scope));
   // Attach the logic operators performing the copies.
   for (size_t k = 0; k < redefined.size(); ++k) {
      const IU& old_iu = *to_redefine[k];
      const IU& new_iu = redefined[k];
      auto logic = ColumnFilterLogic::build(this, pseudo_iu, old_iu, new_iu);
      pipe.attachSuboperator(std::move(scope));
   }
}

}
