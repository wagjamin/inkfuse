#ifndef INKFUSE_KEYRUNTIMESTATE_H
#define INKFUSE_KEYRUNTIMESTATE_H

#include "algebra/suboperators/RuntimeParam.h"
#include <cstdint>

namespace inkfuse {

/// Runtime state needed by all key packing and unpacking operations.
/// A packed key represents multiple IUs being written consecutively into memory.
/// This is for example used for compound keys in aggregations and joins.
struct KeyPackingRuntimeState {
   static const char* name;

   /// Pointer offset within the given key.
   uint16_t offset;
};

/// Runtime state, just with two encoded offsets.
struct KeyPackingRuntimeStateTwo {
   static const char* name;

   /// Pointer offsets within the given key.
   uint16_t offset_1;
   uint16_t offset_2;
};

/// Runtime parameters for key offsets that can be fed into an operator.
/// When a query gets executed, the key layout gets computed and is static
/// for that query. This means that offsets can be hard-baked into the code.
struct KeyPackingRuntimeParams {
   RUNTIME_PARAM(offset, KeyPackingRuntimeState);
};

/// Runtime parameters, just with two encoded offsets.
struct KeyPackingRuntimeParamsTwo {
   RUNTIME_PARAM(offset_1, KeyPackingRuntimeStateTwo);
   RUNTIME_PARAM(offset_2, KeyPackingRuntimeStateTwo);
};

namespace KeyPackingRuntime {
/// Registers the KeyPackingRuntimeState with the InkFuse runtime system.
void registerRuntime();
}

}

#endif //INKFUSE_KEYRUNTIMESTATE_H
