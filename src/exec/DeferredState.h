#ifndef INKFUSE_DEFERREDSTATE_H
#define INKFUSE_DEFERREDSTATE_H

#include "runtime/HashTables.h"
#include "runtime/NewHashTables.h"
#include "runtime/TupleMaterializer.h"
#include <cassert>
#include <deque>

namespace inkfuse {

/// Different suboperator require runtime state to function.
/// We don't know how many threads we will need when building the initial
/// PipelineDAG. Instead, we set up DeferredStateInitializers, and these
/// then set up the required runtime state down the line.
struct DefferredStateInitializer {
   virtual ~DefferredStateInitializer() = default;

   /// Prepare the state once we know how many threads will execute the query.
   virtual void prepare(size_t num_threads) = 0;
   /// Access the thread-local state for a given id.
   virtual void* access(size_t thread_id) = 0;
};

/// Runtime state needed for tuple materializers.
struct TupleMaterializerState : public DefferredStateInitializer {
   TupleMaterializerState(size_t tuple_size_);
   void prepare(size_t num_threads) override;
   void* access(size_t thread_id) override;

   /// The materializers.
   std::deque<TupleMaterializer> materializers;
   /// Read handles for the runtime task that will read from them.
   std::deque<std::unique_ptr<TupleMaterializer::ReadHandle>> handles;
   /// The tuple size.
   size_t tuple_size;
};

/// State needed for an atomic hash table. Supports deferred building.
template <class Comparator>
struct AtomicHashTableState : public DefferredStateInitializer {
   AtomicHashTableState(TupleMaterializerState& materialize_) : materialize(materialize_){};
   void prepare(size_t num_threads) override{};
   void* access(size_t thread_id) override {
      // Called by the actual hash table readers down the line.
      return hash_table.get();
   };

   /// The materializer state from which builder threads read tuples to insert.
   TupleMaterializerState& materialize;
   /// The perfectly sized hash table used by the probe pipeline.
   std::unique_ptr<AtomicHashTable<Comparator>> hash_table;
};

/// Fake object which doesn't defer anything.
template <class State>
struct FakeDefer : public DefferredStateInitializer {
   /// Perfect forwarding constructor to the actual object.
   template <typename... Args>
   FakeDefer(Args&&... args) : object(std::forward<Args>(args)...) {}

   void prepare(size_t num_threads) override{};
   void* access(size_t thread_id) override { return &object; };
   State& obj() { return object; };

   State object;
};

/// Simple key hash table.
struct HashTableSimpleKeyState : public DefferredStateInitializer {
   HashTableSimpleKeyState(size_t key_size_, size_t payload_size_) : key_size(key_size_), payload_size(payload_size_){};

   void prepare(size_t num_threads) override {
      for (size_t k = 0; k < num_threads; ++k) {
         hash_tables.push_back(std::make_unique<HashTableSimpleKey>(key_size, payload_size, 8));
      }
   };

   void* access(size_t thread_id) override {
      assert(thread_id < hash_tables.size());
      return hash_tables[thread_id].get();
   };

   size_t key_size;
   size_t payload_size;
   std::deque<std::unique_ptr<HashTableSimpleKey>> hash_tables;
};

/// Complex key hash table.
struct HashTableComplexKeyState : public DefferredStateInitializer {
   HashTableComplexKeyState(uint16_t slots_, size_t payload_size_) : slots(slots_), payload_size(payload_size_){};

   void prepare(size_t num_threads) override {
      for (size_t k = 0; k < num_threads; ++k) {
         hash_tables.push_back(std::make_unique<HashTableComplexKey>(0, 1, payload_size, 8));
      }
   };

   void* access(size_t thread_id) override {
      assert(thread_id < hash_tables.size());
      return hash_tables[thread_id].get();
   };

   size_t slots;
   size_t payload_size;
   std::deque<std::unique_ptr<HashTableComplexKey>> hash_tables;
};

/// Direct lookup hash table.
struct HashTableDirectLookupState : public DefferredStateInitializer {
   HashTableDirectLookupState(uint16_t payload_size_) : payload_size(payload_size_){};

   void prepare(size_t num_threads) override {
      for (size_t k = 0; k < num_threads; ++k) {
         hash_tables.push_back(std::make_unique<HashTableDirectLookup>(payload_size));
      }
   };

   void* access(size_t thread_id) override {
      assert(thread_id < hash_tables.size());
      return hash_tables[thread_id].get();
   };

   uint16_t payload_size;
   std::deque<std::unique_ptr<HashTableDirectLookup>> hash_tables;
};

} // namespace inkfuse

#endif // INKFUSE_DEFERREDSTATE_H
