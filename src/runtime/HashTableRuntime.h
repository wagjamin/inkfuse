#ifndef INKFUSE_HASHTABLERUNTIME_H
#define INKFUSE_HASHTABLERUNTIME_H

#include "codegen/Type.h"

namespace inkfuse {

/// Both aggregations, as well as joins need to use hash tables in order to compute their result sets.
/// Inkfuse makes this possible by exposing a HashTableRuntime to the generated code.
namespace HashTableRuntime {

extern "C" char* ht_sk_insert(void* table, char* key);
extern "C" char* ht_sk_lookup(void* table, char* key);
extern "C" char* ht_sk_lookup_or_insert(void* table, char* key);
extern "C" void ht_sk_lookup_or_insert_with_init(void* table, char** result, bool* is_new_key, char* key);
extern "C" void ht_sk_it_advance(void* table, char** it_data, uint64_t* it_idx);

/// Special lookup function if we know we have a 0-byte key.
extern "C" char* ht_nk_lookup(void* table);

void registerRuntime();
};


}

#endif //INKFUSE_HASHTABLERUNTIME_H
