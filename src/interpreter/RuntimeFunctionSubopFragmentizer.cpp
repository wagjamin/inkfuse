#include "interpreter/RuntimeFunctionSubopFragmentizer.h"
#include "algebra/suboperators/RuntimeFunctionSubop.h"

namespace inkfuse {

RuntimeFunctionSubopFragmentizer::RuntimeFunctionSubopFragmentizer() {
   // Fragmentize hash table lookup.
   auto& [name, pipe] = pipes.emplace_back();
   const auto& iu = generated_ius.emplace_back(IR::UnsignedInt::build(8));
   const auto& op = pipe.attachSuboperator(RuntimeFunctionSubop::htLookup(nullptr, iu, nullptr));
   name = op.id();
}

}