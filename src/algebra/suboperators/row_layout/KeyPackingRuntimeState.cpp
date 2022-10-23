#include "algebra/suboperators/row_layout/KeyPackingRuntimeState.h"
#include "runtime/Runtime.h"

namespace inkfuse {

const char* KeyPackingRuntimeState::name = "RuntimeKeyExpressionState";
const char* KeyPackingRuntimeStateTwo::name = "RuntimeKeyExpressionStateTwo";

void KeyPackingRuntime::registerRuntime() {
   RuntimeStructBuilder{KeyPackingRuntimeState::name}
      .addMember("offset", IR::UnsignedInt::build(2));

   RuntimeStructBuilder{KeyPackingRuntimeStateTwo::name}
      .addMember("offset_1", IR::UnsignedInt::build(2))
      .addMember("offset_2", IR::UnsignedInt::build(2));
}

}
