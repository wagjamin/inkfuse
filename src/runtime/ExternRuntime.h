#ifndef INKFUSE_EXTHASHTABLERUNTIME_H
#define INKFUSE_EXTHASHTABLERUNTIME_H

#include <cstdint>

/// Exported symbols from the InkFuse runtime relating to hash tables.

namespace inkfuse {

namespace HashTableRuntime {

/// Pure hashing functions.
extern "C" uint64_t hash(void* in, uint64_t len);
extern "C" uint64_t hash4(void* in);
extern "C" uint64_t hash8(void* in);

extern "C" char* ht_sk_insert(void* table, char* key);
extern "C" char* ht_sk_lookup(void* table, char* key);
extern "C" char* ht_sk_lookup_disable(void* table, char* key);
extern "C" char* ht_sk_lookup_or_insert(void* table, char* key);
extern "C" void ht_sk_lookup_or_insert_with_init(void* table, char** result, bool* is_new_key, char* key);
extern "C" void ht_sk_it_advance(void* table, char** it_data, uint64_t* it_idx);

extern "C" char* ht_ck_lookup(void* table, char* key);
extern "C" char* ht_ck_lookup_or_insert(void* table, char* key);
extern "C" void ht_ck_it_advance(void* table, char** it_data, uint64_t* it_idx);

extern "C" char* ht_dl_lookup(void* table, char* key);
extern "C" char* ht_dl_lookup_or_insert(void* table, char* key);
extern "C" void ht_dl_it_advance(void* table, char** it_data, uint64_t* it_idx);

/// Atomic hash-table lookup. No insert needed as that's done by the runtime system.
extern "C" char* ht_at_sk_lookup(void* table, char* key);
extern "C" char* ht_at_sk_lookup_disable(void* table, char* key);
extern "C" char* ht_at_ck_lookup(void* table, char* key);

extern "C" void ht_at_sk_it_advance(void* table, char** it_data, uint64_t* it_idx);

/// Hash/prefetch instructions, fixed width specializations exist.
extern "C" uint64_t ht_at_sk_compute_hash_and_prefetch(void* table, char* key);
extern "C" uint64_t ht_at_sk_compute_hash_and_prefetch_fixed_4(void* table, char* key);
extern "C" uint64_t ht_at_sk_compute_hash_and_prefetch_fixed_8(void* table, char* key);

extern "C" void ht_at_sk_slot_prefetch(void* table, uint64_t hash);
extern "C" char* ht_at_sk_lookup_with_hash(void* table, char* key, uint64_t hash);
extern "C" char* ht_at_sk_lookup_with_hash_disable(void* table, char* key, uint64_t hash);
extern "C" char* ht_at_sk_lookup_with_hash_outer(void* table, char* key, uint64_t hash);

extern "C" uint64_t ht_at_ck_compute_hash_and_prefetch(void* table, char* key);
extern "C" void ht_at_ck_slot_prefetch(void* table, uint64_t hash);
extern "C" char* ht_at_ck_lookup_with_hash(void* table, char* key, uint64_t hash);
extern "C" char* ht_at_ck_lookup_with_hash_disable(void* table, char* key, uint64_t hash);

/// Special lookup function if we know we have a 0-byte key.
extern "C" char* ht_nk_lookup(void* table);
} // namespace HashTableRuntime

namespace TupleMaterializerRuntime {

extern "C" char* materialize_tuple(void* materializer);

}

namespace MemoryRuntime {
extern "C" void* inkfuse_malloc(uint64_t size);
} // namespace MemroyRuntime

} // namespace inkfuse

#endif // INKFUSE_EXTSHTABLERUNTIME_H
