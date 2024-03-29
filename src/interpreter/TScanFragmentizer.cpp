#include "interpreter/TScanFragmentizer.h"
#include "interpreter/FragmentGenerator.h"

namespace inkfuse {

namespace {

// Tscans go over all types.
const std::vector<IR::TypeArc> types =
   TypeDecorator()
      .attachTypes()
      .attachStringType()
      .produce();
}

TScanFragmetizer::TScanFragmetizer() {
   for (auto& type : types) {
      // A table scan fragments consists of a tscan loop driver and the IU provider afterwards.
      auto& [name, pipe] = pipes.emplace_back();
      auto& op = pipe.attachSuboperator(TScanDriver::build(nullptr));
      const auto& driver_iu = **op.getIUs().begin();
      auto& provider_iu = generated_ius.emplace_back(type, "");
      auto& iu_op = pipe.attachSuboperator(TScanIUProvider::build(nullptr, driver_iu, provider_iu));
      // The table scan is uniquely identified by the id of the provider.
      name = iu_op.id();
   }
}

}