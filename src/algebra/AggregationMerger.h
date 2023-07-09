#ifndef INKFUSE_AGGREGATIONMERGER_H
#define INKFUSE_AGGREGATIONMERGER_H

#include "exec/ExecutionContext.h"
#include "runtime/HashTables.h"
#include <deque>

namespace inkfuse {

struct DefferredStateInitializer;
struct Aggregation;

template <class HashTableType>
struct ExclusiveHashTableState;
template <>
struct ExclusiveHashTableState<HashTableSimpleKey>;
template <>
struct ExclusiveHashTableState<HashTableComplexKey>;
template <>
struct ExclusiveHashTableState<HashTableDirectLookup>;

/// The aggregation merger takes a set of aggregate hash tables and merges them.
/// In InkFuse aggregations are multithreaded by doing a thread-local pre-aggregation.
/// Since the same key can be stored in multiple thread-local hash tables, we need
/// to merge the state.
///
/// At the moment this merge phase is completely interpreted, which isn't as efficient
/// as JIT compiling the code for it. In principle both approaches are feasible, so if
/// this ever becomes performance critical we can move away from interpretation to code
/// generation in this phase.
template <class HashTableType>
struct AggregationMerger {
   AggregationMerger(const Aggregation& agg_, ExclusiveHashTableState<HashTableType>& deferred_init_);

   void prepareState(ExecutionContext& ctx, size_t num_threads);
   void mergeTables(ExecutionContext& ctx, size_t thread_id);

   enum class MergeStrategy {
      NoMergeRequired,
      MultiThreaded,
   };

   private:
   void mergeSingleTable(ExecutionContext& ctx, HashTableType& src, HashTableType& target, size_t thread_id);

   const Aggregation& agg;
   ExclusiveHashTableState<HashTableType>& rt_state;
   std::deque<std::unique_ptr<HashTableType>> pre_merge;
   size_t total_threads;
   MergeStrategy strategy;
};

} // namespace inkfuse

#endif //INKFUSE_AGGREGATIONMERGER_H
