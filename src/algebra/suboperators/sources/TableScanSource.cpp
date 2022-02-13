#include "algebra/suboperators/sources/TableScanSource.h"
#include "algebra/CompilationContext.h"
#include "algebra/RelAlgOp.h"
#include "codegen/Type.h"
#include "exec/FuseChunk.h"
#include <functional>

namespace inkfuse {

std::unique_ptr<TScanDriver> TScanDriver::build(const RelAlgOp* source) {
   return std::unique_ptr<TScanDriver>(new TScanDriver(source));
}

TScanDriver::TScanDriver(const RelAlgOp* source)
   : LoopDriver<TScanDriverRuntimeParams>(source) {
}

bool TScanDriver::pickMorsel() {
   assert(state);

   if (!first_picked) {
      state->start = 0;
      first_picked = true;
   } else {
      state->start = state->end;
   }

   // Go up to the maximum chunk size of the intermediate results or the total relation size.
   state->end = std::min(state->start + DEFAULT_CHUNK_SIZE, params->rel_size);

   // If the starting point advanced to the end, then we know there are no more morsels to pick.
   return state->start != params->rel_size;
}

std::string TScanDriver::id() const
{
   return "TScanDriver";
}

void TScanIUProvider::setUpStateImpl(const ExecutionContext& context) {
   state->raw_data = params->raw_data;
}

std::string TScanIUProvider::providerName() const {
   return "TScanIUProvider";
}

std::unique_ptr<TScanIUProvider> TScanIUProvider::build(const RelAlgOp* source, const IU& driver_iu, const IU& produced_iu) {
   return std::unique_ptr<TScanIUProvider>(new TScanIUProvider{source, driver_iu, produced_iu});
}

TScanIUProvider::TScanIUProvider(const RelAlgOp* source, const IU& driver_iu, const IU& produced_iu)
   : IndexedIUProvider<TScanIUProviderRuntimeParams>(source, driver_iu, produced_iu) {
}

}