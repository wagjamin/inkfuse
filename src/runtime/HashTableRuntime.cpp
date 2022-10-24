#include "runtime/HashTableRuntime.h"
#include "runtime/HashTables.h"
#include "runtime/Runtime.h"

namespace inkfuse {

namespace {
// Lower order 63 bits in the hash slot store the hash suffix.
const uint64_t fill_mask = 1ull << 63ull;
// Highest order bit in the hash slot indicates if slot is filled.
const uint64_t hash_mask = fill_mask - 1;
}

extern "C" char* HashTableRuntime::ht_sk_lookup(void* table, char* key) {
   return reinterpret_cast<HashTableSimpleKey*>(table)->lookup(key);
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


void HashTableRuntime::registerRuntime() {
   RuntimeFunctionBuilder("ht_sk_lookup", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()));

   RuntimeFunctionBuilder("ht_sk_lookup_or_insert", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()));

   RuntimeFunctionBuilder("ht_sk_lookup_or_insert_with_init", IR::Void::build())
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("result", IR::Pointer::build(IR::Pointer::build(IR::Char::build())))
      .addArg("is_new_key", IR::Pointer::build(IR::Bool::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()));

   RuntimeFunctionBuilder("ht_sk_it_advance", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("it_data", IR::Pointer::build(IR::Pointer::build(IR::Char::build())))
      .addArg("it_idx", IR::Pointer::build(IR::UnsignedInt::build(8)));

   RuntimeFunctionBuilder("ht_nk_lookup", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()));
}

}