#include "runtime/HashRuntime.h"
#include "runtime/Runtime.h"
#include "xxhash.h"

namespace inkfuse {

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
