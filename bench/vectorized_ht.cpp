#include "benchmark/benchmark.h"
#include "runtime/NewHashTables.h"
#include "xxhash.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <vector>

using namespace inkfuse;

/**
 * Microbenchmarks inspired by Peter's feedback: In vectorized engines,
 * parallel hash table access can be made more efficient than in a tuple-at-a time
 * codegen engine.
 * The reason is that it's easier to generate parallel, independent main memory loads.
 * This is especially important when the hash tables exceed the cache size.
 *
 * The benchmark contains a tuple at a time hash table resolution microbenchmark,
 * and a more vectorized flavor that goes over a single batch in three steps:
 * 1. Hash Table Slot Computation
 * 2. Hash Table Slot Prefetch
 * 3. Tuple At A Time Collision Resolution
 *
 * Our experiments show that while for small hash tables, the results are negligible
 * and both approaches are doing similar, for large hash tables the vectorized implementation
 * performs much better.
 * At a few million rows, are are able to consistently get ~50%+ higher tuple throughput
 * in the vectorized implementation:
 *
 * Running ./inkbench
 * Run on (16 X 4800 MHz CPU s)
 * CPU Caches:
 *  L1 Data 32K (x8)
 *  L1 Instruction 32K (x8)
 *  L2 Unified 256K (x8)
 *  L3 Unified 16384K (x1)
 * Load Average: 0.90, 0.46, 0.35
 * ------------------------------------------------------------------------------------
 * Benchmark                          Time             CPU   Iterations UserCounters...
 * ------------------------------------------------------------------------------------
 * BM_ht_perf_tat/512               5530 ns         5527 ns       125991 items_per_second=92.6339M/s
 * BM_ht_perf_tat/8192            130711 ns       130651 ns         5366 items_per_second=62.7012M/s
 * BM_ht_perf_tat/32768           568894 ns       568885 ns         1231 items_per_second=57.6004M/s
 * BM_ht_perf_tat/524288        13101426 ns     13100658 ns           54 items_per_second=40.02M/s
 * BM_ht_perf_tat/33554432    1590459650 ns   1589748687 ns            1 items_per_second=21.1068M/s
 * BM_ht_perf_tat/1073741824 77115560402 ns  77105227736 ns            1 items_per_second=13.9257M/s
 * BM_ht_perf_vectorized/512/256               5469 ns         5467 ns       127657 items_per_second=93.6571M/s
 * BM_ht_perf_vectorized/8192/256            128080 ns       128023 ns         5461 items_per_second=63.9885M/s
 * BM_ht_perf_vectorized/32768/256           534317 ns       534303 ns         1295 items_per_second=61.3285M/s
 * BM_ht_perf_vectorized/524288/256        10098416 ns     10093265 ns           72 items_per_second=51.9443M/s
 * BM_ht_perf_vectorized/33554432/256     971872286 ns    971838853 ns            1 items_per_second=34.5267M/s
 * BM_ht_perf_vectorized/1073741824/256 51425526675 ns  51422464322 ns            1 items_per_second=20.8808M/s
 *
 */
namespace {

/**
 * Hash table used for benchmarking. Very simple, single threaded.
 */
template <class KeyType, class ValueType>
struct BenchmarkHashTable {
   /// Key size could be computed using sizeof(KeyType). But the explicit
   /// key types makes this more similar to the hash tables we have in InkFuse.
   BenchmarkHashTable(size_t capacity_, size_t key_size_)
      : key_size(key_size_), capacity(capacity_), entries(capacity) {
   }

   // No separate zero slot - important for a real HT, but not for the microbenchmarks here.
   struct Entry {
      KeyType key = 0;
      ValueType value = 0;
   };

   void tat_insert(const KeyType& key, const ValueType& value) {
      const auto hash = XXH3_64bits(&key, key_size);
      auto slot_idx = hash % capacity;
      Entry* entry = &entries[slot_idx];
      while (entry->key != 0) {
         slot_idx = (slot_idx + 1) % capacity;
         entry = &entries[slot_idx];
      }
      entry->key = key;
      entry->value = value;
   }

   const Entry* tat_lookup(const KeyType& key) const {
      const auto hash = XXH3_64bits(&key, key_size);
      auto slot_idx = hash % capacity;
      const Entry* entry = &entries[slot_idx];
      while (entry->key != 0) {
         if (std::memcmp(static_cast<const void*>(&key), static_cast<const void*>(&entry->key), key_size) == 0) {
            return entry;
         }
         slot_idx = (slot_idx + 1) % capacity;
         entry = &entries[slot_idx];
      }
      return nullptr;
   };

   const uint64_t vec_slot(const KeyType& key) const {
      const auto hash = XXH3_64bits(&key, key_size);
      return hash % capacity;
   };

   const void vec_load(const uint64_t& slot_idx) const {
      __builtin_prefetch(&entries[slot_idx]);
   };

   const uint64_t vec_slot_and_load(const KeyType& key) const {
      const auto hash = XXH3_64bits(&key, key_size);
      const auto slot = hash % capacity;
      __builtin_prefetch(&entries[slot]);
      return slot;
   };

   const Entry* vec_lookup(const KeyType& key, uint64_t slot_idx) const {
      const Entry* entry = &entries[slot_idx];
      while (entry->key != 0) {
         if (std::memcmp(static_cast<const void*>(&key), static_cast<const void*>(&entry->key), key_size) == 0) {
            return entry;
         }
         slot_idx = (slot_idx + 1) % capacity;
         entry = &entries[slot_idx];
      }
      return nullptr;
   };

   size_t key_size;
   size_t capacity;
   std::vector<Entry> entries;
};

/**
 * Tuple-at-a time hash lookup like in a vectorized engine.
 */
void BM_ht_perf_tat(benchmark::State& state) {
   // Add param values to the hash table.
   const uint64_t num_elems = state.range(0);
   BenchmarkHashTable<uint64_t, uint64_t> ht{static_cast<size_t>(num_elems) * 2, 8};
   for (uint64_t k = 1; k <= num_elems; ++k) {
      const uint64_t lookup_key = 7 * k;
      ht.tat_insert(lookup_key, k);
   }

   for (auto _ : state) {
      // Lookup every key again.
      for (uint64_t k = 1; k <= num_elems; ++k) {
         const uint64_t lookup_key = 7 * k;
         const auto* res = ht.tat_lookup(lookup_key);
         // We have to do something with the result, otherwise the compiler is too smart
         // to optimize memory accesses away.
         if (res->value > num_elems) {
            throw std::runtime_error("bad ht lookup for " + std::to_string(k));
         }
      }
   }
   state.SetItemsProcessed(state.iterations() * num_elems);
}

/**
 * Fancier vectorized lookup primitive meant to create lots of
 * independent loads.
 */
void BM_ht_perf_vectorized(benchmark::State& state) {
   const uint64_t num_elems = state.range(0);
   const uint64_t batch_size = state.range(1);
   BenchmarkHashTable<uint64_t, uint64_t> ht{static_cast<size_t>(num_elems) * 2, 8};
   for (uint64_t k = 1; k <= num_elems; ++k) {
      ht.tat_insert(7 * k, k);
   }
   std::vector<uint64_t> keys(batch_size);
   std::vector<uint64_t> slots(batch_size);
   for (auto _ : state) {
      // Lookup every key again.
      for (uint64_t k = 1; k <= num_elems; k += batch_size) {
         const auto curr_batch = std::min(batch_size, num_elems - k + 1);
         for (uint64_t tid = 0; tid < curr_batch; ++tid) {
            keys[tid] = 7 * (k + tid);
         }
         for (uint64_t tid = 0; tid < curr_batch; ++tid) {
            slots[tid] = ht.vec_slot(keys[tid]);
         }
         for (uint64_t tid = 0; tid < curr_batch; ++tid) {
            ht.vec_load(slots[tid]);
         }
         for (uint64_t tid = 0; tid < curr_batch; ++tid) {
            const auto* res = ht.vec_lookup(keys[tid], slots[tid]);
            // We have to do something with the result, otherwise the compiler is too smart
            // to optimize memory accesses away.
            if (res->value > num_elems) {
               throw std::runtime_error("bad ht lookup for " + std::to_string(k));
            }
         }
      }
   }
   state.SetItemsProcessed(state.iterations() * num_elems);
}

/**
 * Vectorized hash table as in the ROF paper. Fused prefetching & hash
 * computation to overlap loads and computation nicely.
 */
void BM_ht_perf_vectorized_rof(benchmark::State& state) {
   const uint64_t num_elems = state.range(0);
   const uint64_t batch_size = state.range(1);
   BenchmarkHashTable<uint64_t, uint64_t> ht{static_cast<size_t>(num_elems) * 2, 8};
   for (uint64_t k = 1; k <= num_elems; ++k) {
      ht.tat_insert(7 * k, k);
   }
   std::vector<uint64_t> keys(batch_size);
   std::vector<uint64_t> slots(batch_size);
   for (auto _ : state) {
      // Lookup every key again.
      for (uint64_t k = 1; k <= num_elems; k += batch_size) {
         const auto curr_batch = std::min(batch_size, num_elems - k + 1);
         for (uint64_t tid = 0; tid < curr_batch; ++tid) {
            keys[tid] = 7 * (k + tid);
         }
         for (uint64_t tid = 0; tid < curr_batch; ++tid) {
            slots[tid] = ht.vec_slot_and_load(keys[tid]);
         }
         for (uint64_t tid = 0; tid < curr_batch; ++tid) {
            const auto* res = ht.vec_lookup(keys[tid], slots[tid]);
            // We have to do something with the result, otherwise the compiler is too smart
            // to optimize memory accesses away.
            if (res->value > num_elems) {
               throw std::runtime_error("bad ht lookup for " + std::to_string(k));
            }
         }
      }
   }
   state.SetItemsProcessed(state.iterations() * num_elems);
}

void BM_ht_perf_tat_inkfuse(benchmark::State& state) {
   const uint64_t num_elems = state.range(0);
   inkfuse::SimpleKeyComparator comp{8};
   AtomicHashTable<inkfuse::SimpleKeyComparator> ht{comp, 16, 2 * num_elems};
   for (uint64_t k = 1; k <= num_elems; ++k) {
      const uint64_t key = 7 * k;
      char* value = ht.insert<true>(reinterpret_cast<const char*>(&key));
      reinterpret_cast<uint64_t*>(value)[1] = k;
   }
   for (auto _ : state) {
      for (uint64_t k = 1; k <= num_elems; ++k) {
         const uint64_t key = 7 * k;
         char* res = ht.lookup(reinterpret_cast<const char*>(&key));
         if (reinterpret_cast<const uint64_t*>(res)[1] > num_elems) {
            throw std::runtime_error("bad ht lookup for " + std::to_string(k));
         }
      }
   }
   state.SetItemsProcessed(state.iterations() * num_elems);
}

void BM_ht_perf_vectorized_inkfuse(benchmark::State& state) {
   const uint64_t num_elems = state.range(0);
   const uint64_t batch_size = state.range(1);
   inkfuse::SimpleKeyComparator comp{8};
   AtomicHashTable<inkfuse::SimpleKeyComparator> ht{comp, 16, 2 * num_elems};
   for (uint64_t k = 1; k <= num_elems; ++k) {
      const uint64_t key = 7 * k;
      char* value = ht.insert<true>(reinterpret_cast<const char*>(&key));
      reinterpret_cast<uint64_t*>(value)[1] = k;
   }
   std::vector<uint64_t> keys(batch_size);
   std::vector<uint64_t> hashes(batch_size);
   std::vector<char*> results(batch_size);
   for (auto _ : state) {
      // Lookup every key again.
      for (uint64_t k = 1; k <= num_elems; k += batch_size) {
         const auto curr_batch = std::min(batch_size, num_elems - k + 1);
         for (uint64_t tid = 0; tid < curr_batch; ++tid) {
            keys[tid] = 7 * (k + tid);
         }
         for (uint64_t tid = 0; tid < curr_batch; ++tid) {
            hashes[tid] = ht.compute_hash_and_prefetch(reinterpret_cast<const char*>(&keys[tid]));
         }
         for (uint64_t tid = 0; tid < curr_batch; ++tid) {
            results[tid] = ht.lookup(reinterpret_cast<const char*>(&keys[tid]), hashes[tid]);
         }
      }
   }
   state.SetItemsProcessed(state.iterations() * num_elems);
}

BENCHMARK(BM_ht_perf_tat)->Arg(1 << 9)->Arg(1 << 13)->Arg(1 << 15)->Arg(1 << 19)->Arg(1 << 21)->Arg(1 << 25)->Arg(1 << 30);
BENCHMARK(BM_ht_perf_tat_inkfuse)->Arg(1 << 9)->Arg(1 << 13)->Arg(1 << 15)->Arg(1 << 19)->Arg(1 << 21)->Arg(1 << 25)->Arg(1 << 30);

BENCHMARK(BM_ht_perf_vectorized)->ArgPair(1 << 9, 256)->ArgPair(1 << 13, 256)->ArgPair(1 << 15, 256)->ArgPair(1 << 19, 256)->ArgPair(1 << 21, 256)->ArgPair(1 << 25, 256)->ArgPair(1 << 30, 256);
// Different internal batch sizes. 256 is a good value.
BENCHMARK(BM_ht_perf_vectorized)->ArgPair(1 << 25, 64)->ArgPair(1 << 25, 128)->ArgPair(1 << 25, 256)->ArgPair(1 << 25, 512)->ArgPair(1 << 25, 1024)->ArgPair(1 << 25, 2024)->ArgPair(1 << 25, 4048)->ArgPair(1 << 25, 8096)->ArgPair(1 << 25, 16192);

BENCHMARK(BM_ht_perf_vectorized_rof)->ArgPair(1 << 9, 256)->ArgPair(1 << 13, 256)->ArgPair(1 << 15, 256)->ArgPair(1 << 19, 256)->ArgPair(1 << 21, 256)->ArgPair(1 << 25, 256)->ArgPair(1 << 30, 256);
// Different internal batch sizes. 256 is a good value.
BENCHMARK(BM_ht_perf_vectorized_rof)->ArgPair(1 << 25, 64)->ArgPair(1 << 25, 128)->ArgPair(1 << 25, 256)->ArgPair(1 << 25, 512)->ArgPair(1 << 25, 1024)->ArgPair(1 << 25, 2024)->ArgPair(1 << 25, 4048)->ArgPair(1 << 25, 8096)->ArgPair(1 << 25, 16192);

BENCHMARK(BM_ht_perf_vectorized_inkfuse)->ArgPair(1 << 9, 256)->ArgPair(1 << 13, 256)->ArgPair(1 << 15, 256)->ArgPair(1 << 19, 256)->ArgPair(1 << 21, 256)->ArgPair(1 << 25, 256)->ArgPair(1 << 30, 256);
// Different internal batch sizes. 256 is a good value.
BENCHMARK(BM_ht_perf_vectorized_inkfuse)->ArgPair(1 << 25, 64)->ArgPair(1 << 25, 128)->ArgPair(1 << 25, 256)->ArgPair(1 << 25, 512)->ArgPair(1 << 25, 1024)->ArgPair(1 << 25, 2024)->ArgPair(1 << 25, 4048)->ArgPair(1 << 25, 8096)->ArgPair(1 << 25, 16192);

} // namespace
