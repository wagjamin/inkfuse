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

   // Compute the size of the merge tables into which every thread will write the merged tuples.
   const size_t merge_table_size = HashTableType::getMergeTableSize(pre_merge, num_threads);
   rt_state.merge_table_size = merge_table_size;
   rt_state.hash_tables.resize(num_threads);

   // Store information required for the later merge phase.
   strategy = MergeStrategy::MultiThreaded;
   total_threads = num_threads;
}

template <class HashTableType>
void AggregationMerger<HashTableType>::mergeTables(ExecutionContext& ctx, size_t thread_id) {
   if (strategy == MergeStrategy::NoMergeRequired) {
      return;
   }

   const size_t merge_table_size = rt_state.merge_table_size;

   // Only perform the hash table allocation once we are in the multi-threaded phase. We used to allocate
   // all hash tables in the prepare phase, but this caused excessive page faulting & zero setting on a single thread.
   if constexpr (std::is_same_v<HashTableType, HashTableSimpleKey>) {
      HashTableSimpleKey& original_table = *pre_merge[0];
      rt_state.hash_tables[thread_id] = std::make_unique<HashTableSimpleKey>(original_table.keySize(), original_table.payloadSize(), merge_table_size);
   } else if constexpr (std::is_same_v<HashTableType, HashTableComplexKey>) {
      HashTableComplexKey& original_table = *pre_merge[0];
      rt_state.hash_tables[thread_id] = std::make_unique<HashTableComplexKey>(original_table.simpleKeySize(), original_table.complexKeySlots(), original_table.payloadSize(), merge_table_size);
   } else {
      HashTableDirectLookup& original_table = *pre_merge[0];
      rt_state.hash_tables[thread_id] = std::make_unique<HashTableDirectLookup>(original_table.slot_size - 2);
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
   do {
      ExecutionContext::getInstalledRestartFlag() = false;
      while (it_data != nullptr) {
         const uint64_t slot_hash = src.computeHash(it_data);
         if (slot_hash % total_threads == thread_id) {
            // This thread is responsible for merging this tuple.
            merge_pairs.emplace_back(it_data, target.lookupOrInsert(it_data));
         }
         src.iteratorAdvance(&it_data, &it_idx);
      }
      // It could happen that we resized the hash table during the insert. This usually shouldn't happen.
   } while (ExecutionContext::getInstalledRestartFlag());
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

// More efficient merge implementation for the HashTableSimpleKey.
template <>
void AggregationMerger<HashTableSimpleKey>::mergeSingleTable(ExecutionContext& ctx, HashTableSimpleKey& src, HashTableSimpleKey& target, size_t thread_id) {
   const size_t total_threads = ctx.getNumThreads();
   char* it_data;
   uint64_t next_jump_point;
   uint64_t it_idx;
   src.partitionedIteratorStart(&it_data, &it_idx, &next_jump_point, total_threads, thread_id);
   std::vector<std::pair<const char*, char*>> merge_pairs;
   merge_pairs.reserve(src.size());
   ExecutionContext::RuntimeGuard guard{ctx, thread_id};
   assert(!ExecutionContext::getInstalledRestartFlag());
   // First move over the keys into the merge table.
   do {
      ExecutionContext::getInstalledRestartFlag() = false;
      while (it_data != nullptr) {
         const uint64_t slot_hash = src.computeHash(it_data);
         if (slot_hash % total_threads == thread_id) {
            // This thread is responsible for merging this tuple.
            merge_pairs.emplace_back(it_data, target.lookupOrInsert(it_data));
         }
         src.partitionedIteratorAdvance(&it_data, &it_idx, &next_jump_point, total_threads, thread_id);
      }
      // It could happen that we resized the hash table during the insert. This usually shouldn't happen.
   } while (ExecutionContext::getInstalledRestartFlag());
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
