#ifndef INKFUSE_DEFERREDSTATE_H
#define INKFUSE_DEFERREDSTATE_H

#include "runtime/HashTables.h"
#include "runtime/NewHashTables.h"
#include "runtime/TupleMaterializer.h"
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

/// Simple key hash table - nothing is deferred here.
struct HashTableSimpleKeyState : public FakeDefer<HashTableSimpleKey> {
   HashTableSimpleKeyState(size_t key_size, size_t payload_size) : FakeDefer(key_size, payload_size, 8){};
};

/// Complex key hash table - nothing is deferred here.
struct HashTableComplexKeyState : public FakeDefer<HashTableComplexKey> {
   HashTableComplexKeyState(uint16_t slots, size_t payload_size) : FakeDefer(0, slots, payload_size, 8){};
};

/// Direct lookup hash table - nothing is deferred here.
struct HashTableDirectLookupState : public FakeDefer<HashTableDirectLookup> {
   HashTableDirectLookupState(size_t payload_size) : FakeDefer(payload_size){};
};

} // namespace inkfuse

#endif // INKFUSE_DEFERREDSTATE_H
