#include "interpreter/RuntimeFunctionSubopFragmentizer.h"
#include "algebra/suboperators/RuntimeFunctionSubop.h"
#include "runtime/NewHashTables.h"

namespace inkfuse {

RuntimeFunctionSubopFragmentizer::RuntimeFunctionSubopFragmentizer() {
   // Hash tables operations are defined on all types.
   // For char* and ByteArrays, the input IU is not dereferenced, for all other types it is.
   auto in_types = TypeDecorator().attachTypes().attachPackedKeyTypes().produce();
   // For inserts we have a separate primitive that doesn't return anything. Needed for join builds
   // without payloads.
   auto out_types = std::vector<IR::TypeArc>{IR::Pointer::build(IR::Char::build()), nullptr};
   for (const auto& in_type : in_types) {
      // Fragmentize hash table lookup.
      {
         auto& [name, pipe] = pipes.emplace_back();
         const auto& key = generated_ius.emplace_back(in_type);
         const auto& result_ptr = generated_ius.emplace_back(IR::Pointer::build(IR::Char::build()));
         // No pseudo-IU inputs, these only matter for more complex DAGs.
         const auto& op = pipe.attachSuboperator(RuntimeFunctionSubop::htLookup<HashTableSimpleKey>(nullptr, result_ptr, key, {}));
         name = op.id();
      }
      {
         auto& [name, pipe] = pipes.emplace_back();
         const auto& key = generated_ius.emplace_back(in_type);
         const auto& result_ptr = generated_ius.emplace_back(IR::Pointer::build(IR::Char::build()));
         // No pseudo-IU inputs, these only matter for more complex DAGs.
         const auto& op = pipe.attachSuboperator(RuntimeFunctionSubop::htLookup<HashTableDirectLookup>(nullptr, result_ptr, key, {}));
         name = op.id();
      }
      {
         auto& [name, pipe] = pipes.emplace_back();
         const auto& key = generated_ius.emplace_back(in_type);
         const auto& result_ptr = generated_ius.emplace_back(IR::Pointer::build(IR::Char::build()));
         // No pseudo-IU inputs, these only matter for more complex DAGs.
         const auto& op = pipe.attachSuboperator(RuntimeFunctionSubop::htLookup<AtomicHashTable<SimpleKeyComparator>>(nullptr, result_ptr, key, {}));
         name = op.id();
      }

      // Fragmentize Vectorized Hash Table Primitives
      {
         // Hash and prefetch:
         auto& [name, pipe] = pipes.emplace_back();
         const auto& key = generated_ius.emplace_back(in_type);
         const auto& hash = generated_ius.emplace_back(IR::UnsignedInt::build(8));
         const auto& op = pipe.attachSuboperator(RuntimeFunctionSubop::htHashAndPrefetch<AtomicHashTable<SimpleKeyComparator>>(nullptr, hash, key, {}));
         name = op.id();
      }
      {
         // Lookup don't disable slot:
         auto& [name, pipe] = pipes.emplace_back();
         const auto& hash = generated_ius.emplace_back(IR::UnsignedInt::build(8));
         const auto& key = generated_ius.emplace_back(in_type);
         const auto& result = generated_ius.emplace_back(IR::Pointer::build(IR::Char::build()));
         const auto& op = pipe.attachSuboperator(RuntimeFunctionSubop::htLookupWithHash<AtomicHashTable<SimpleKeyComparator>, false>(nullptr, result, key, hash, nullptr));
         name = op.id();
      }
      {
         // Lookup disable slot:
         auto& [name, pipe] = pipes.emplace_back();
         const auto& hash = generated_ius.emplace_back(IR::UnsignedInt::build(8));
         const auto& key = generated_ius.emplace_back(in_type);
         const auto& result = generated_ius.emplace_back(IR::Pointer::build(IR::Char::build()));
         const auto& op = pipe.attachSuboperator(RuntimeFunctionSubop::htLookupWithHash<AtomicHashTable<SimpleKeyComparator>, true>(nullptr, result, key, hash, nullptr));
         name = op.id();
      }

      for (const auto& out_type : out_types) {
         // Fragmentize hash table insert.
         {
            auto& [name, pipe] = pipes.emplace_back();
            const auto& key = generated_ius.emplace_back(in_type);
            const IU* out_iu = nullptr;
            if (out_type) {
               out_iu = &generated_ius.emplace_back(out_type);
            }
            // No pseudo-IU inputs, these only matter for more complex DAGs.
            const auto& op = pipe.attachSuboperator(RuntimeFunctionSubop::htInsert(nullptr, out_iu, key, {}));
            name = op.id();
         }

         // Fragmentize hash table lookup with insert.
         {
            auto& [name, pipe] = pipes.emplace_back();
            const auto& key = generated_ius.emplace_back(in_type);
            const IU* out_iu = nullptr;
            if (out_type) {
               out_iu = &generated_ius.emplace_back(out_type);
            }
            // No pseudo-IU inputs, these only matter for more complex DAGs.
            const auto& op = pipe.attachSuboperator(RuntimeFunctionSubop::htLookupOrInsert<HashTableSimpleKey>(nullptr, out_iu, key, {}));
            name = op.id();
         }
         {
            auto& [name, pipe] = pipes.emplace_back();
            const auto& key = generated_ius.emplace_back(in_type);
            const IU* out_iu = nullptr;
            if (out_type) {
               out_iu = &generated_ius.emplace_back(out_type);
            }
            // No pseudo-IU inputs, these only matter for more complex DAGs.
            const auto& op = pipe.attachSuboperator(RuntimeFunctionSubop::htLookupOrInsert<HashTableDirectLookup>(nullptr, out_iu, key, {}));
            name = op.id();
         }
      }
   }

   // Fragmentize tuple materialization.
   {
      auto& [name, pipe] = pipes.emplace_back();
      const auto& input = generated_ius.emplace_back(IR::Char::build());
      const auto& result_ptr = generated_ius.emplace_back(IR::Pointer::build(IR::Char::build()));
      const auto& op = pipe.attachSuboperator(RuntimeFunctionSubop::materializeTuple(nullptr, result_ptr, {&input}, nullptr));
      name = op.id();
   }

   // Fragmentize no-key hash table lookup/insert. Does not care about
   // input types at all. The input IU just makes connecting the DAG easier.
   // We still create a 1-byte input type as that's the only thing that really lets
   // us guarantee that we don't try to access a fuse chunk out of bounds.
   {
      auto& [name, pipe] = pipes.emplace_back();
      const auto& input = generated_ius.emplace_back(IR::Char::build());
      const auto& result_ptr = generated_ius.emplace_back(IR::Pointer::build(IR::Char::build()));
      const auto& op = pipe.attachSuboperator(RuntimeFunctionSubop::htNoKeyLookup(nullptr, result_ptr, input, nullptr));
      name = op.id();
   }

   // Fragmentize string insert on the complex hash table.
   {
      auto& [name, pipe] = pipes.emplace_back();
      const auto& key = generated_ius.emplace_back(IR::String::build());
      const auto& result_ptr = generated_ius.emplace_back(IR::Pointer::build(IR::Char::build()));
      // No pseudo-IU inputs, these only matter for more complex DAGs.
      const auto& op = pipe.attachSuboperator(RuntimeFunctionSubop::htLookup<HashTableComplexKey>(nullptr, result_ptr, key, {}));
      name = op.id();
   }

   // Fragmentize hash table lookup that disables the slot (for left semi joins).
   {
      auto& [name, pipe] = pipes.emplace_back();
      const auto& key = generated_ius.emplace_back(IR::String::build());
      const auto& result_ptr = generated_ius.emplace_back(IR::Pointer::build(IR::Char::build()));
      // No pseudo-IU inputs, these only matter for more complex DAGs.
      const auto& op = pipe.attachSuboperator(RuntimeFunctionSubop::htLookupOrInsert<HashTableComplexKey>(nullptr, &result_ptr, key, {}));
      name = op.id();
   }
}

}
