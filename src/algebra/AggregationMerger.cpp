#include "algebra/AggregationMerger.h"
#include "algebra/Aggregation.h"
#include "exec/DeferredState.h"
#include "runtime/HashTables.h"
#include <vector>

namespace inkfuse {

namespace {

// Merge primitive for aggregate sum state.
template <typename T>
void mergeSum(std::vector<std::pair<const char*, char*>> pairs, size_t offset) {
   for (size_t k = 0; k < pairs.size(); ++k) {
      const T* src = reinterpret_cast<const T*>(pairs[k].first + offset);
      T* dest = reinterpret_cast<T*>(pairs[k].second + offset);
      *dest += *src;
   }
}

}

template <class HashTableType>
AggregationMerger<HashTableType>::AggregationMerger(const Aggregation& agg_, ExclusiveHashTableState<HashTableType>& deferred_init_) : agg(agg_), rt_state(deferred_init_) {
}

template <class HashTableType>
void AggregationMerger<HashTableType>::prepareState(ExecutionContext&, size_t num_threads) {
   if (num_threads == 1) {
      // There is only one hash table, we don't need to do any merging as we know
      // we are duplicate free.
      strategy = MergeStrategy::NoMergeRequired;
      return;
   }

   pre_merge = std::move(rt_state.hash_tables);

   // Set up the merge tables into which every thread will write the merged tuples.
   std::deque<std::unique_ptr<HashTableType>> merged = HashTableType::buildMergeTables(pre_merge, num_threads);
   rt_state.hash_tables = std::move(merged);

   // Store information required for the later merge phase.
   strategy = MergeStrategy::MultiThreaded;
   total_threads = num_threads;
}

template <class HashTableType>
void AggregationMerger<HashTableType>::mergeTables(ExecutionContext& ctx, size_t thread_id) {
   if (strategy == MergeStrategy::NoMergeRequired) {
      return;
   }

   auto& merge_into = rt_state.hash_tables[thread_id];
   for (auto& merge_from : pre_merge) {
      // Merge every non-partitioned source table into the target.
      mergeSingleTable(ctx, *merge_from, *merge_into, thread_id);
   }
}

template <class HashTableType>
void AggregationMerger<HashTableType>::mergeSingleTable(ExecutionContext& ctx, HashTableType& src, HashTableType& target, size_t thread_id) {
   char* it_data;
   uint64_t it_idx;
   src.iteratorStart(&it_data, &it_idx);
   std::vector<std::pair<const char*, char*>> merge_pairs;
   merge_pairs.reserve(src.size());
   ExecutionContext::RuntimeGuard guard{ctx, thread_id};
   assert(!ExecutionContext::getInstalledRestartFlag());
   // First move over the keys into the merge table.
   while (it_data != nullptr) {
      const uint64_t slot_hash = src.computeHash(it_data);
      if (slot_hash % total_threads == thread_id) {
         // This thread is responsible for merging this tuple.
         merge_pairs.emplace_back(it_data, target.lookupOrInsert(it_data));
      }
      src.iteratorAdvance(&it_data, &it_idx);
   }
   if (ExecutionContext::getInstalledRestartFlag()) {
      // We resized the hash table during the insert. This usually shouldn't happen.
      // TODO(benjamin): Supporting this is easy - just go through the hash tables once more.
      throw std::runtime_error("Hash table grew during mergeSingleTable()");
   }
   ExecutionContext::getInstalledRestartFlag() = false;
   // Now perform the actual merge based on the previously identified pointers.
   size_t curr_offset = agg.key_size + agg.payload_offset;
   for (const auto& [agg_iu, agg_state] : agg.granules) {
      const IR::TypeArc& agg_type = agg_iu->type;
      // Dispatch onto the right merge primitive.
      if (agg_type->id() == "UI4") {
         mergeSum<uint32_t>(merge_pairs, curr_offset);
      } else if (agg_type->id() == "UI8") {
         mergeSum<uint64_t>(merge_pairs, curr_offset);
      } else if (agg_type->id() == "I4") {
         mergeSum<int32_t>(merge_pairs, curr_offset);
      } else if (agg_type->id() == "I8") {
         mergeSum<int64_t>(merge_pairs, curr_offset);
      } else if (agg_type->id() == "F4") {
         mergeSum<float>(merge_pairs, curr_offset);
      } else if (agg_type->id() == "F8") {
         mergeSum<double>(merge_pairs, curr_offset);
      } else if (agg_state->id() == "agg_state_count") {
         // Count can go over any type - it always has a summable 8 byte integer state.
         mergeSum<int64_t>(merge_pairs, curr_offset);
      } else {
         throw std::runtime_error("Unsupported merge type for aggregate hash table");
      }
      // Update the offset - the next aggregation state granule lives next to this one.
      curr_offset += agg_state->getStateSize();
   }
}

// Declare all specializations.
template class AggregationMerger<HashTableSimpleKey>;
template class AggregationMerger<HashTableComplexKey>;
template class AggregationMerger<HashTableDirectLookup>;

} // namespace inkfuse
