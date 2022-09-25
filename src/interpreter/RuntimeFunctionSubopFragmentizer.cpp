#include "interpreter/RuntimeFunctionSubopFragmentizer.h"
#include "algebra/suboperators/RuntimeFunctionSubop.h"

namespace inkfuse {

RuntimeFunctionSubopFragmentizer::RuntimeFunctionSubopFragmentizer() {
   // Hash tables operations are defined on char* and ByteArrays.
   std::vector<IR::TypeArc> in_types = {IR::Pointer::build(IR::Char::build()), IR::ByteArray::build(8)};
   for (auto& in_type : in_types) {
      // Fragmentize hash table lookup.
      {
         auto& [name, pipe] = pipes.emplace_back();
         const auto& key = generated_ius.emplace_back(in_type);
         const auto& result_ptr = generated_ius.emplace_back(IR::Pointer::build(IR::Char::build()));
         // No pseudo-IU inputs, these only matter for more complex DAGs.
         const auto& op = pipe.attachSuboperator(RuntimeFunctionSubop::htLookup(nullptr, result_ptr, key, {}));
         name = op.id();
      }

      // Fragmentize hash table lookup with insert.
      {
         auto& [name, pipe] = pipes.emplace_back();
         const auto& key = generated_ius.emplace_back(in_type);
         const auto& result_ptr = generated_ius.emplace_back(IR::Pointer::build(IR::Char::build()));
         // No pseudo-IU inputs, these only matter for more complex DAGs.
         const auto& op = pipe.attachSuboperator(RuntimeFunctionSubop::htLookupOrInsert(nullptr, result_ptr, key, {}));
         name = op.id();
      }
   }
}

}