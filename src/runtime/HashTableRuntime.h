#ifndef INKFUSE_HASHTABLERUNTIME_H
#define INKFUSE_HASHTABLERUNTIME_H

#include "codegen/Type.h"

namespace inkfuse {

/// Both aggregations, as well as joins need to use hash tables in order to compute their result sets.
/// Inkfuse makes this possible by exposing a HashTableRuntime to the generated code.
namespace HashTableRuntime {
void registerRuntime();
} // namespace HashTableRuntime

namespace TupleMaterializerRuntime {
void registerRuntime();
} // namespace TupleMaterializerRuntime

namespace MemoryRuntime {
void registerRuntime();
} // namespace MemoryRuntime

} // namespace inkfuse

#endif //INKFUSE_HASHTABLERUNTIME_H
