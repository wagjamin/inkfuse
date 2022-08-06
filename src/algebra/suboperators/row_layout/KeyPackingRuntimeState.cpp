#include "algebra/suboperators/row_layout/KeyPackingRuntimeState.h"
#include "runtime/Runtime.h"

namespace inkfuse {

const char* KeyPackingRuntimeState::name = "RuntimeKeyExpressionState";

void KeyPackingRuntime::registerRuntime() {
   RuntimeStructBuilder{KeyPackingRuntimeState::name}
      .addMember("offset", IR::UnsignedInt::build(2));
}

}
