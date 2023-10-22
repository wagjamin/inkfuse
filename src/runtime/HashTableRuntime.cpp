#include "runtime/HashTableRuntime.h"
#include "runtime/HashTables.h"
#include "runtime/NewHashTables.h"
#include "runtime/Runtime.h"

namespace inkfuse {

extern "C" char* HashTableRuntime::ht_sk_insert(void* table, char* key) {
   return reinterpret_cast<HashTableSimpleKey*>(table)->insert(key);
}

extern "C" char* HashTableRuntime::ht_sk_lookup(void* table, char* key) {
   return reinterpret_cast<HashTableSimpleKey*>(table)->lookup(key);
}

extern "C" char* HashTableRuntime::ht_sk_lookup_disable(void* table, char* key) {
   return reinterpret_cast<HashTableSimpleKey*>(table)->lookupDisable(key);
}

extern "C" char* HashTableRuntime::ht_sk_lookup_or_insert(void* table, char* key) {
   return reinterpret_cast<HashTableSimpleKey*>(table)->lookupOrInsert(key);
}

extern "C" void HashTableRuntime::ht_sk_lookup_or_insert_with_init(void* table, char** result, bool* is_new_key, char* key) {
   reinterpret_cast<HashTableSimpleKey*>(table)->lookupOrInsert(result, is_new_key, key);
}

extern "C" void HashTableRuntime::ht_sk_it_advance(void* table, char** it_data, uint64_t* it_idx) {
   reinterpret_cast<HashTableSimpleKey*>(table)->iteratorAdvance(it_data, it_idx);
}

extern "C" char* HashTableRuntime::ht_nk_lookup(void* table) {
   return reinterpret_cast<HashTableSimpleKey*>(table)->lookupOrInsertSingleKey();
}

extern "C" char* HashTableRuntime::ht_ck_lookup(void* table, char* key) {
   return reinterpret_cast<HashTableComplexKey*>(table)->lookup(key);
}

extern "C" char* HashTableRuntime::ht_ck_lookup_or_insert(void* table, char* key) {
   return reinterpret_cast<HashTableComplexKey*>(table)->lookupOrInsert(key);
}

extern "C" void HashTableRuntime::ht_ck_it_advance(void* table, char** it_data, uint64_t* it_idx) {
   reinterpret_cast<HashTableComplexKey*>(table)->iteratorAdvance(it_data, it_idx);
}

extern "C" char* HashTableRuntime::ht_dl_lookup(void* table, char* key) {
   return reinterpret_cast<HashTableDirectLookup*>(table)->lookup(key);
}

extern "C" char* HashTableRuntime::ht_dl_lookup_or_insert(void* table, char* key) {
   return reinterpret_cast<HashTableDirectLookup*>(table)->lookupOrInsert(key);
}

extern "C" void HashTableRuntime::ht_dl_it_advance(void* table, char** it_data, uint64_t* it_idx) {
   reinterpret_cast<HashTableDirectLookup*>(table)->iteratorAdvance(it_data, it_idx);
}

// Atomic hash table.
extern "C" uint64_t HashTableRuntime::ht_at_sk_compute_hash(void* table, char* key) {
   return reinterpret_cast<AtomicHashTable<SimpleKeyComparator>*>(table)->compute_hash(key);
}

extern "C" void HashTableRuntime::ht_at_sk_slot_prefetch(void* table, uint64_t hash) {
   reinterpret_cast<AtomicHashTable<SimpleKeyComparator>*>(table)->slot_prefetch(hash);
}

extern "C" char* HashTableRuntime::ht_at_sk_lookup_with_hash(void* table, char* key, uint64_t hash) {
   return reinterpret_cast<AtomicHashTable<SimpleKeyComparator>*>(table)->lookup(key, hash);
}

extern "C" char* HashTableRuntime::ht_at_sk_lookup_with_hash_disable(void* table, char* key, uint64_t hash) {
   return reinterpret_cast<AtomicHashTable<SimpleKeyComparator>*>(table)->lookupDisable(key, hash);
}

extern "C" uint64_t HashTableRuntime::ht_at_ck_compute_hash(void* table, char* key) {
   return reinterpret_cast<AtomicHashTable<ComplexKeyComparator>*>(table)->compute_hash(key);
}

extern "C" void HashTableRuntime::ht_at_ck_slot_prefetch(void* table, uint64_t hash) {
   reinterpret_cast<AtomicHashTable<ComplexKeyComparator>*>(table)->slot_prefetch(hash);
}

extern "C" char* HashTableRuntime::ht_at_ck_lookup_with_hash(void* table, char* key, uint64_t hash) {
   return reinterpret_cast<AtomicHashTable<ComplexKeyComparator>*>(table)->lookup(key, hash);
}

extern "C" char* HashTableRuntime::ht_at_ck_lookup_with_hash_disable(void* table, char* key, uint64_t hash) {
   return reinterpret_cast<AtomicHashTable<ComplexKeyComparator>*>(table)->lookupDisable(key, hash);
}

extern "C" char* HashTableRuntime::ht_at_sk_lookup(void* table, char* key) {
   return reinterpret_cast<AtomicHashTable<SimpleKeyComparator>*>(table)->lookup(key);
}

extern "C" char* HashTableRuntime::ht_at_sk_lookup_disable(void* table, char* key) {
   return reinterpret_cast<AtomicHashTable<SimpleKeyComparator>*>(table)->lookupDisable(key);
}

extern "C" char* HashTableRuntime::ht_at_ck_lookup(void* table, char* key) {
   return reinterpret_cast<AtomicHashTable<ComplexKeyComparator>*>(table)->lookup(key);
}

void HashTableRuntime::registerRuntime() {
   RuntimeFunctionBuilder("ht_sk_insert", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_sk_lookup", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()), true)
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_sk_lookup_disable", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_sk_lookup_or_insert", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_sk_lookup_or_insert_with_init", IR::Void::build())
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("result", IR::Pointer::build(IR::Pointer::build(IR::Char::build())))
      .addArg("is_new_key", IR::Pointer::build(IR::Bool::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_sk_it_advance", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()), true)
      .addArg("it_data", IR::Pointer::build(IR::Pointer::build(IR::Char::build())))
      .addArg("it_idx", IR::Pointer::build(IR::UnsignedInt::build(8)));

   RuntimeFunctionBuilder("ht_nk_lookup", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()), true);

   RuntimeFunctionBuilder("ht_ck_lookup", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()), true)
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_ck_lookup_or_insert", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_ck_it_advance", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()), true)
      .addArg("it_data", IR::Pointer::build(IR::Pointer::build(IR::Char::build())))
      .addArg("it_idx", IR::Pointer::build(IR::UnsignedInt::build(8)));

   RuntimeFunctionBuilder("ht_dl_lookup", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()), true)
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_dl_lookup_or_insert", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_dl_it_advance", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()), true)
      .addArg("it_data", IR::Pointer::build(IR::Pointer::build(IR::Char::build())))
      .addArg("it_idx", IR::Pointer::build(IR::UnsignedInt::build(8)));

   // Atomic hash table.
   RuntimeFunctionBuilder("ht_at_sk_lookup", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_at_sk_lookup_disable", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_at_ck_lookup", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_at_sk_compute_hash", IR::UnsignedInt::build(8))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_at_sk_slot_prefetch", IR::Void::build())
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("hash", IR::UnsignedInt::build(8), true);

   RuntimeFunctionBuilder("ht_at_sk_lookup_with_hash", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()))
      .addArg("hash", IR::UnsignedInt::build(8), true);

   RuntimeFunctionBuilder("ht_at_sk_lookup_with_hash_disable", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()))
      .addArg("hash", IR::UnsignedInt::build(8), true);

   RuntimeFunctionBuilder("ht_at_ck_compute_hash", IR::UnsignedInt::build(8))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_at_ck_slot_prefetch", IR::Void::build())
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("hash", IR::UnsignedInt::build(8), true);

   RuntimeFunctionBuilder("ht_at_ck_lookup_with_hash", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()))
      .addArg("hash", IR::UnsignedInt::build(8), true);

   RuntimeFunctionBuilder("ht_at_ck_lookup_with_hash_disable", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()))
      .addArg("hash", IR::UnsignedInt::build(8), true);
}

}
