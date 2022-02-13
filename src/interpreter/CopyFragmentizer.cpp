#include "interpreter/CopyFragmentizer.h"
#include "algebra/suboperators/Copy.h"

namespace inkfuse {

namespace {
const std::vector<IR::TypeArc> types =
   TypeDecorator()
      .attachIntegers()
      .produce();
}

CopyFragmentizer::CopyFragmentizer()
{
   for (auto& type: types) {
      // A copy fragments consists only of a copy suboperator.
      auto& [name, pipe] = pipes.emplace_back();
      auto& iu = generated_ius.emplace_back(type, "");
      pipe.attachSuboperator(Copy::build(iu));
   }
}

const std::list<std::pair<std::string, Pipeline>>& CopyFragmentizer::getFragments() const
{
   return pipes;
}

}
