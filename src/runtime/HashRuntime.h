#ifndef INKFUSE_HASHRUNTIME_H
#define INKFUSE_HASHRUNTIME_H

#include <cstddef>
#include <cstdint>
#include "xxhash.h"
namespace inkfuse {

/// Hash runtime allowing generated code to hash values. We currently
/// use xxhash as hash function.
/// There are two ways we could go for this, and it's not 100% clear what is better:
/// 1. Include xxhash directly in the generated code. This way it can make use of inline hints etc.
/// 2. Link xxhash statically into inkfuse and expose it through the inkfuse runtime.
/// The second option makes it a lot easier to ship inkfuse binaries, and we are going with
/// it for now until it becomes a performance bottleneck.
/// (Also note that the include-style option (1) only works nicely in a world where we generate C).
/// TODO Narrower types should use narrower hashes, but fine for now. No point to compute 64 bit hashes on 32 bit data.
namespace HashRuntime {

extern "C" uint64_t hash(void* in, uint64_t len);
extern "C" uint64_t hash4(void* in);
extern "C" uint64_t hash8(void* in);

void registerRuntime();

}


}

#endif //INKFUSE_HASHRUNTIME_H
