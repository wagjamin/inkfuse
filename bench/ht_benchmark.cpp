#include "benchmark/benchmark.h"
#include "runtime/HashTables.h"
#include "xxhash.h"
#include <cstring>
#include <random>
#include <unordered_map>
#include <array>

/// This microbenchmark measures our "Key-Aware" hash table.
/// To minimize the complexity of the suboperators, it makes
/// sense to provide a hash-table that is key-aware and can resolve
/// collisions by itself.
/// This comes at the cost of increased runtime overhead because
/// now there is some interpretation overhead within the hash
/// table lookup functions.
namespace inkfuse {

namespace {

const size_t random_seed = 42;
// Every probe test does 100 million probes on varying HT size.
const size_t probe_count = 100'000'000;

// XX hash hasher we also use in the std::unordered_map to not measure
// the overhead of xxhash (std::hash is the identity).
template <typename T>
struct XXHasher {
   std::size_t operator()(T const& t) const noexcept {
      return XXH3_64bits(&t, sizeof(t));
   }
};

struct RandomData {
   std::vector<char> keys;
   std::vector<char> payloads;
};

RandomData buildData(size_t num_elems, size_t key_size, size_t payload_size, size_t seed = random_seed) {
   using RandomBits = std::independent_bits_engine<std::mt19937, 8, unsigned short>;
   RandomBits bits(seed);
   std::vector<char> keys;
   keys.resize(key_size * num_elems);
   for (char& elem : keys) {
      auto bit = bits();
      elem = static_cast<char>(bit);
   }
   std::vector<char> payloads;
   payloads.resize(payload_size * num_elems);
   for (char& elem : payloads) {
      elem = static_cast<char>(bits());
   }
   return {.keys = std::move(keys), .payloads = std::move(payloads)};
};

void addToKeyAwareHT(RandomData& data, HashTableSimpleKey& ht, size_t num_elems, size_t key_size, size_t payload_size) {
   char* key = &data.keys[0];
   char* payload = &data.payloads[0];
   for (size_t k = 0; k < num_elems; ++k) {
      auto payload_ptr = ht.lookupOrInsert(key);
      std::memcpy(payload_ptr, payload, payload_size);
      key += key_size;
      payload += payload_size;
   }
}

void probeKeyAwareHT(RandomData& data, HashTableSimpleKey& ht, size_t num_elems, size_t key_size) {
   for (size_t k = 0; k < probe_count; ++k) {
      auto idx = k % num_elems;
      char* key = &data.keys[idx * key_size];
      ht.lookup(key);
   }
}

template <class Map>
void addToUnorderedMap(RandomData& data, Map& ht, size_t num_elems, size_t key_size, size_t payload_size) {
   char* key_ptr = &data.keys[0];
   char* payload_ptr = &data.payloads[0];
   for (size_t k = 0; k < num_elems; ++k) {
      typename Map::key_type* key = &reinterpret_cast<typename Map::key_type&>(key_ptr);
      typename Map::mapped_type* payload = &reinterpret_cast<typename Map::mapped_type&>(payload_ptr);
      ht.emplace(*key, *payload);
      key_ptr += key_size;
      payload_ptr += payload_size;
   }
}

template <class Map>
void probeUnorderedMap(RandomData& data, Map& ht, size_t num_elems, size_t key_size) {
   for (size_t k = 0; k < probe_count; ++k) {
      auto idx = k % num_elems;
      char* key_ptr = &data.keys[idx * key_size];
      typename Map::key_type* key = &reinterpret_cast<typename Map::key_type&>(key_ptr);
      ht.find(*key);
   }
}

void ht_insert_key_aware(benchmark::State& state) {
   const auto num_elems = state.range(0);
   const auto key_size = state.range(1);
   const auto payload_size = state.range(2);
   auto data = buildData(num_elems, key_size, payload_size);
   for (auto _ : state) {
      HashTableSimpleKey ht(key_size, payload_size);
      addToKeyAwareHT(data, ht, num_elems, key_size, payload_size);
   }
   state.SetItemsProcessed(state.iterations() * num_elems);
   state.SetBytesProcessed(state.iterations() * num_elems * (key_size + payload_size));
}

void ht_lookup_key_aware_match(benchmark::State& state) {
   const auto num_elems = state.range(0);
   const auto key_size = state.range(1);
   const auto payload_size = state.range(2);
   auto data = buildData(num_elems, key_size, payload_size);
   HashTableSimpleKey ht(key_size, payload_size);
   addToKeyAwareHT(data, ht, num_elems, key_size, payload_size);
   for (auto _ : state) {
      probeKeyAwareHT(data, ht, num_elems, key_size);
   }
   state.SetItemsProcessed(state.iterations() * probe_count);
}

void ht_lookup_key_aware_nomatch(benchmark::State& state) {
   const auto num_elems = state.range(0);
   const auto key_size = state.range(1);
   const auto payload_size = state.range(2);
   auto data = buildData(num_elems, key_size, payload_size);
   auto data_nomatch = buildData(num_elems, key_size, payload_size, 420);
   HashTableSimpleKey ht(key_size, payload_size);
   addToKeyAwareHT(data, ht, num_elems, key_size, payload_size);
   for (auto _ : state) {
      probeKeyAwareHT(data_nomatch, ht, num_elems, key_size);
   }
   state.SetItemsProcessed(state.iterations() * probe_count);
}

template <class KV_pair>
void ht_insert_unordered_map(benchmark::State& state) {
   using KeyT = typename KV_pair::first_type;
   using ValueT = typename KV_pair::second_type;
   const auto num_elems = state.range(0);
   const auto key_size = sizeof(KeyT);
   const auto payload_size = sizeof(ValueT);
   auto data = buildData(num_elems, key_size, payload_size);
   for (auto _ : state) {
      std::unordered_map<KeyT, ValueT, XXHasher<KeyT>> ht;
      addToUnorderedMap(data, ht, num_elems, key_size, payload_size);
   }
   state.SetItemsProcessed(state.iterations() * num_elems);
   state.SetBytesProcessed(state.iterations() * num_elems * (key_size + payload_size));
}

template <class KV_pair>
void ht_lookup_unordered_map_match(benchmark::State& state) {
   using KeyT = typename KV_pair::first_type;
   using ValueT = typename KV_pair::second_type;
   const auto num_elems = state.range(0);
   const auto key_size = sizeof(KeyT);
   const auto payload_size = sizeof(ValueT);
   auto data = buildData(num_elems, key_size, payload_size);
   std::unordered_map<KeyT, ValueT, XXHasher<KeyT>> ht;
   addToUnorderedMap(data, ht, num_elems, key_size, payload_size);
   for (auto _ : state) {
      probeUnorderedMap(data, ht, num_elems, key_size);
   }
   state.SetItemsProcessed(state.iterations() * probe_count);
}

template <class KV_pair>
void ht_lookup_unordered_map_nomatch(benchmark::State& state) {
   using KeyT = typename KV_pair::first_type;
   using ValueT = typename KV_pair::second_type;
   const auto num_elems = state.range(0);
   const auto key_size = sizeof(KeyT);
   const auto payload_size = sizeof(ValueT);
   auto data = buildData(num_elems, key_size, payload_size);
   auto data_nomatch = buildData(num_elems, key_size, payload_size, 420);
   std::unordered_map<KeyT, ValueT, XXHasher<KeyT>> ht;
   addToUnorderedMap(data, ht, num_elems, key_size, payload_size);
   for (auto _ : state) {
      probeUnorderedMap(data_nomatch, ht, num_elems, key_size);
   }
   state.SetItemsProcessed(state.iterations() * probe_count);
}

using TB8 = std::array<char, 8>;
using TB32 = std::array<char, 32>;
using TB64 = std::array<char, 64>;

// InkFuse Simple Key Map Insert Benchmarks.
// Insert 1k, 100k, 10M, 50M keys.
// Key sizes: 8, 32, 64 bytes.
// Payload sizes: 8, 64 bytes.
BENCHMARK(ht_insert_key_aware)->ArgsProduct({{1'000, 100'000, 10'000'000, 50'000'000}, {8, 32, 64}, {8, 64}});

// std::unordered_map Insert Benchmarks.
// Insert 1k, 100k, 10M, 50M keys.
// Key sizes: 8, 32, 64 bytes.
// Payload sizes: 8, 64 bytes.
BENCHMARK(ht_insert_unordered_map<std::pair<TB8, TB8>>)->ArgsProduct({{1'000, 100'000, 10'000'000, 50'000'000}});
BENCHMARK(ht_insert_unordered_map<std::pair<TB8, TB64>>)->ArgsProduct({{1'000, 100'000, 10'000'000, 50'000'000}});
BENCHMARK(ht_insert_unordered_map<std::pair<TB32, TB8>>)->ArgsProduct({{1'000, 100'000, 10'000'000, 50'000'000}});
BENCHMARK(ht_insert_unordered_map<std::pair<TB32, TB64>>)->ArgsProduct({{1'000, 100'000, 10'000'000, 50'000'000}});
BENCHMARK(ht_insert_unordered_map<std::pair<TB64, TB8>>)->ArgsProduct({{1'000, 100'000, 10'000'000, 50'000'000}});
BENCHMARK(ht_insert_unordered_map<std::pair<TB64, TB64>>)->ArgsProduct({{1'000, 100'000, 10'000'000, 50'000'000}});

// InkFuse Simple Key Map Lookup Benchmarks.
// Insert 1k, 100k, 10M, 50M keys.
// Key sizes: 8, 32, 64 bytes.
// Payload sizes: 8, 64 bytes.
BENCHMARK(ht_lookup_key_aware_match)->ArgsProduct({{1'000, 100'000, 10'000'000, 50'000'000}, {8, 32, 64}, {8, 64}});
BENCHMARK(ht_lookup_key_aware_nomatch)->ArgsProduct({{1'000, 100'000, 10'000'000, 50'000'000}, {8, 32, 64}, {8, 64}});

// std::unordered_map Lookup Benchmarks.
// Insert 1k, 100k, 10M, 50M keys.
// Key sizes: 8, 32, 64 bytes.
// Payload sizes: 8, 64 bytes.
BENCHMARK(ht_lookup_unordered_map_match<std::pair<TB8, TB8>>)->ArgsProduct({{1'000, 100'000, 10'000'000, 50'000'000}});
BENCHMARK(ht_lookup_unordered_map_match<std::pair<TB8, TB64>>)->ArgsProduct({{1'000, 100'000, 10'000'000, 50'000'000}});
BENCHMARK(ht_lookup_unordered_map_match<std::pair<TB32, TB8>>)->ArgsProduct({{1'000, 100'000, 10'000'000, 50'000'000}});
BENCHMARK(ht_lookup_unordered_map_match<std::pair<TB32, TB64>>)->ArgsProduct({{1'000, 100'000, 10'000'000, 50'000'000}});
BENCHMARK(ht_lookup_unordered_map_match<std::pair<TB64, TB8>>)->ArgsProduct({{1'000, 100'000, 10'000'000, 50'000'000}});
BENCHMARK(ht_lookup_unordered_map_match<std::pair<TB64, TB64>>)->ArgsProduct({{1'000, 100'000, 10'000'000, 50'000'000}});
BENCHMARK(ht_lookup_unordered_map_nomatch<std::pair<TB8, TB8>>)->ArgsProduct({{1'000, 100'000, 10'000'000, 50'000'000}});
BENCHMARK(ht_lookup_unordered_map_nomatch<std::pair<TB8, TB64>>)->ArgsProduct({{1'000, 100'000, 10'000'000, 50'000'000}});
BENCHMARK(ht_lookup_unordered_map_nomatch<std::pair<TB32, TB8>>)->ArgsProduct({{1'000, 100'000, 10'000'000, 50'000'000}});
BENCHMARK(ht_lookup_unordered_map_nomatch<std::pair<TB32, TB64>>)->ArgsProduct({{1'000, 100'000, 10'000'000, 50'000'000}});
BENCHMARK(ht_lookup_unordered_map_nomatch<std::pair<TB64, TB8>>)->ArgsProduct({{1'000, 100'000, 10'000'000, 50'000'000}});
BENCHMARK(ht_lookup_unordered_map_nomatch<std::pair<TB64, TB64>>)->ArgsProduct({{1'000, 100'000, 10'000'000, 50'000'000}});

}

}