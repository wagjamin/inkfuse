#ifndef INKFUSE_HASHTABLERUNTIME_H
#define INKFUSE_HASHTABLERUNTIME_H

#include "codegen/Type.h"

namespace inkfuse {

/// Both aggregations, as well as joins need to use hash tables in order to compute their result sets.
/// Inkfuse makes this possible by exposing a HashTableRuntime to the generated code.
///
/// In order to work in an incremental fusion engine, the hash table needs to support batched and
/// non-batched operations. A hash table is set up through the following parameters:
///     - RowLayout describes the key types within the table.
///     - PayloadSize describes how many bytes the payload should have.
///
/// For non-batched operations, the following interfaces are provided:
/// lookup(uint64_t hash) -> void* data
/// lookupNext(void* data) -> void* data
/// insert(uint64_t hash) -> void* data
///
/// For batched operations, the following interfaces are provided:
/// lookupBatch(void** data, uint64_t* hashes, uint64_t* keys)
struct HashTableRuntime {

   using RowLayout = std::vector<TypeArc>;


};

}

#endif //INKFUSE_HASHTABLERUNTIME_H
