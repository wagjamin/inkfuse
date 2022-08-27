#include "interpreter/RuntimeFunctionSubopFragmentizer.h"
#include "algebra/suboperators/RuntimeFunctionSubop.h"

namespace inkfuse {

RuntimeFunctionSubopFragmentizer::RuntimeFunctionSubopFragmentizer() {
   // Fragmentize hash table lookup.
   {
      auto& [name, pipe] = pipes.emplace_back();
      const auto& key = generated_ius.emplace_back(IR::Pointer::build(IR::Char::build()));
      const auto& result_ptr = generated_ius.emplace_back(IR::Pointer::build(IR::Char::build()));
      const auto& op = pipe.attachSuboperator(RuntimeFunctionSubop::htLookup(nullptr, result_ptr, key));
      name = op.id();
   }

   // Fragmentize hash table lookup with insert.
   {
      auto& [name, pipe] = pipes.emplace_back();
      const auto& key = generated_ius.emplace_back(IR::Pointer::build(IR::Char::build()));
      const auto& result_ptr = generated_ius.emplace_back(IR::Pointer::build(IR::Char::build()));
      const auto& op = pipe.attachSuboperator(RuntimeFunctionSubop::htLookupOrInsert(nullptr, result_ptr, key));
      name = op.id();
   }
}

}