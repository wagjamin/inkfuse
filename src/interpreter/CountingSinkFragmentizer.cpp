#include "interpreter/CountingSinkFragmentizer.h"
#include "algebra/suboperators/sinks/CountingSink.h"

namespace inkfuse {

CountingSinkFragmentizer::CountingSinkFragmentizer()
{
   // Only one fragment for the counting sink as it is type independent.
   // Using the char here is a bit abusive - note that we are still generating code of the IU provider
   // as part of this fragment - we just don't use the IU.
   // As a result, we need to make sure the pointer cannot go out of bounds during the access.
   // How can we do that? Easy - just use a type with minimal size - in this case a 1 byte char.
   auto& [name, pipe] = pipes.emplace_back();
   const auto& iu = generated_ius.emplace_back(IR::Char::build());
   const auto& op = pipe.attachSuboperator(CountingSink::build(iu));
   name = op.id();
}

}
