#include "algebra/suboperators/sources/TableScanSource.h"
#include "algebra/CompilationContext.h"
#include "algebra/RelAlgOp.h"
#include "codegen/Type.h"
#include "exec/FuseChunk.h"
#include <algorithm>
#include <functional>

namespace inkfuse {

std::unique_ptr<TScanDriver> TScanDriver::build(const RelAlgOp* source, size_t rel_size_) {
   return std::unique_ptr<TScanDriver>(new TScanDriver(source, rel_size_));
}

TScanDriver::TScanDriver(const RelAlgOp* source, size_t rel_size_)
   : LoopDriver(source), rel_size(rel_size_) {
}

Suboperator::PickMorselResult TScanDriver::pickMorsel(size_t thread_id) {
   assert(states);
   LoopDriverState& state = (*states).at(thread_id);

   size_t morsel_start = start_idx.fetch_add(DEFAULT_CHUNK_SIZE);

   if (morsel_start >= rel_size) {
      // If the starting point advanced beyond the end, then we know there are no more morsels to pick.
      return NoMoreMorsels{};
   }

   // Go up to the maximum chunk size of the intermediate results or the total relation size.
   state.start = morsel_start;
   state.end = std::min(state.start + DEFAULT_CHUNK_SIZE, rel_size);

   return PickedMorsel{
      .morsel_size = state.end - state.start,
      .pipeline_progress = static_cast<double>(state.end) / rel_size,
   };
}

std::string TScanDriver::id() const {
   return "TScanDriver";
}

void TScanIUProvider::setUpStateImpl(const ExecutionContext& context) {
   for (auto& state : *states) {
      state.start = &raw_data;
   }
}

std::string TScanIUProvider::providerName() const {
   return "TScanIUProvider";
}

std::unique_ptr<TScanIUProvider> TScanIUProvider::build(const RelAlgOp* source, const IU& driver_iu, const IU& produced_iu, char* raw_data_) {
   return std::unique_ptr<TScanIUProvider>(new TScanIUProvider{source, driver_iu, produced_iu, raw_data_});
}

TScanIUProvider::TScanIUProvider(const RelAlgOp* source, const IU& driver_iu, const IU& produced_iu, char* raw_data_)
   : IndexedIUProvider(source, driver_iu, produced_iu), raw_data(raw_data_) {
}

}
