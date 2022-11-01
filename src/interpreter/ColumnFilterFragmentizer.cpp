#include "interpreter/ColumnFilterFragmentizer.h"
#include "algebra/suboperators/ColumnFilter.h"

namespace inkfuse {

namespace {

/// Types we actually filter.
const std::vector<IR::TypeArc> types = TypeDecorator().attachTypes().attachStringType().attachPackedKeyTypes().produce();
/// Types that appear within the filter condition.
const std::vector<IR::TypeArc> condition_types = {IR::Bool::build(), IR::Pointer::build(IR::Char::build())};

}

ColumnFilterFragmentizer::ColumnFilterFragmentizer() {
   // Not self filtering.
   for (auto& type: types) {
      for (auto& condition_type: condition_types) {
         auto& [name, pipe] = pipes.emplace_back();
         const auto& filter_iu = generated_ius.emplace_back(condition_type, "filter");
         // Note that the void IU will not be produced in repipeAll, i.e. the fragment does not have a FuseChunkSink.
         const auto& pseudo_iu = generated_ius.emplace_back(IR::Void::build(), "");
         const auto& target_iu = generated_ius.emplace_back(type, "target_iu");
         const auto& target_iu_out = generated_ius.emplace_back(type, "target_iu_filtered");
         pipe.attachSuboperator(ColumnFilterScope::build(nullptr, filter_iu, pseudo_iu));
         auto& filter_subop = pipe.attachSuboperator(ColumnFilterLogic::build(nullptr, pseudo_iu, target_iu, target_iu_out, condition_type, false));
         name = filter_subop.id();
      }
   }
   // Self filtering.
   for (auto& condition_type: condition_types) {
         auto& [name, pipe] = pipes.emplace_back();
         const auto& filter_iu = generated_ius.emplace_back(condition_type, "filter");
         // Note that the void IU will not be produced in repipeAll, i.e. the fragment does not have a FuseChunkSink.
         const auto& pseudo_iu = generated_ius.emplace_back(IR::Void::build(), "");
         const auto& target_iu_out = generated_ius.emplace_back(condition_type, "target_iu_filtered");
         pipe.attachSuboperator(ColumnFilterScope::build(nullptr, filter_iu, pseudo_iu));
         auto& filter_subop = pipe.attachSuboperator(ColumnFilterLogic::build(nullptr, pseudo_iu, filter_iu, target_iu_out, condition_type, true));
         name = filter_subop.id();
   }
}

}
