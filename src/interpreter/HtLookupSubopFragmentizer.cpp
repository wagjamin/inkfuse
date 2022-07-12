#include "interpreter/HtLookupSubopFragmentizer.h"
#include "algebra/suboperators/HtLookupSubop.h"

namespace inkfuse {

HtLookupSubopFragmentizer::HtLookupSubopFragmentizer() {
   auto& [name, pipe] = pipes.emplace_back();
   const auto& iu = generated_ius.emplace_back(IR::UnsignedInt::build(8));
   const auto& op = pipe.attachSuboperator(HtLookupSubop::build(nullptr, iu, nullptr));
   name = op.id();
}

}