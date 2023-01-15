#include "runtime/HashRuntime.h"
#include "runtime/Runtime.h"
#include "xxhash.h"

namespace inkfuse {

extern "C" uint64_t hash(void* in, uint64_t len) {
   return XXH3_64bits(in, len);
}

extern "C" uint64_t hash4(void* in) {
   return XXH3_64bits(in, 4);
}

extern "C" uint64_t hash8(void* in) {
   return XXH3_64bits(in, 8);
}

void HashRuntime::registerRuntime() {
   RuntimeFunctionBuilder("hash", IR::UnsignedInt::build(8))
      .addArg("in", IR::Pointer::build(IR::Void::build()), true)
      .addArg("len", IR::UnsignedInt::build(8));

   RuntimeFunctionBuilder("hash4", IR::UnsignedInt::build(8))
      .addArg("in", IR::Pointer::build(IR::Void::build()), true);

   RuntimeFunctionBuilder("hash8", IR::UnsignedInt::build(8))
      .addArg("in", IR::Pointer::build(IR::Void::build()), true);
}
}