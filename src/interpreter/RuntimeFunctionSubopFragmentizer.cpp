#include "interpreter/RuntimeFunctionSubopFragmentizer.h"
#include "algebra/suboperators/RuntimeFunctionSubop.h"

namespace inkfuse {

RuntimeFunctionSubopFragmentizer::RuntimeFunctionSubopFragmentizer() {
   // Hash tables operations are defined on all types.
   // For char* and ByteArrays, the input IU is not dereferenced, for all other types it is.
   auto in_types = TypeDecorator().attachTypes().attachPackedKeyTypes().produce();
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