#include "interpreter/ColumnFilterFragmentizer.h"
#include "algebra/suboperators/ColumnFilter.h"

namespace inkfuse {

namespace {

const std::vector<IR::TypeArc> types = TypeDecorator().attachTypes().produce();

}

ColumnFilterFragmentizer::ColumnFilterFragmentizer() {
   for (auto& type: types) {
      auto& [name, pipe] = pipes.emplace_back();
      const auto& filter_iu = generated_ius.emplace_back(IR::Bool::build(), "filter");
      // Note that the void IU will not be produced in repipeAll, i.e. the fragment does not have a FuseChunkSink.
      const auto& pseudo_iu = generated_ius.emplace_back(IR::Void::build(), "");
      const auto& target_iu = generated_ius.emplace_back(type, "target_iu");
      const auto& target_iu_out = generated_ius.emplace_back(type, "target_iu_filtered");
      pipe.attachSuboperator(ColumnFilterScope::build(nullptr, filter_iu, pseudo_iu));
      auto& filter_subop = pipe.attachSuboperator(ColumnFilterLogic::build(nullptr, pseudo_iu, target_iu, target_iu_out));
      name = filter_subop.id();
   }
}

}
