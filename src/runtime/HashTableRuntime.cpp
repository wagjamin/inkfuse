#include "runtime/HashTableRuntime.h"
#include "runtime/HashTables.h"
#include "runtime/Runtime.h"
#include <cstring>

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

void HashTableRuntime::registerRuntime() {
   RuntimeFunctionBuilder("ht_sk_lookup", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("hash", IR::Pointer::build(IR::Char::build()));

   RuntimeFunctionBuilder("ht_sk_lookup_or_insert", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("hash", IR::Pointer::build(IR::Char::build()));
}

}