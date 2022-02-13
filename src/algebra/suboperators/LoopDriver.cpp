#include "algebra/suboperators/LoopDriver.h"
#include "runtime/Runtime.h"

namespace inkfuse {

const char* LoopDriverState::name = "LoopDriverState";

void LoopDriverRuntime::registerRuntime() {
   RuntimeStructBuilder{LoopDriverState::name}
      .addMember("start", IR::UnsignedInt::build(8))
      .addMember("end", IR::UnsignedInt::build(8));
}

}
