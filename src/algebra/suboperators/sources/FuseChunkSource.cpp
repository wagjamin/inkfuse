#include "algebra/suboperators/sources/FuseChunkSource.h"
#include "exec/ExecutionContext.h"

namespace inkfuse {

std::unique_ptr<FuseChunkSourceDriver> FuseChunkSourceDriver::build() {
   return std::unique_ptr<FuseChunkSourceDriver>(new FuseChunkSourceDriver());
}

FuseChunkSourceDriver::FuseChunkSourceDriver() : LoopDriver(nullptr) {
}

Suboperator::PickMorselResult FuseChunkSourceDriver::pickMorsel(size_t thread_id) {
   assert(thread_id < cols.size());
   LoopDriverState& state = (*states)[thread_id];
   Column* col = cols[thread_id];
   state.start = 0;
   state.end = col->size;
   // We can always pick a morsel for a fuse chunk. The actual execution is responsible for making
   // sure that pickMorsel for fuse chunks is invoked at the right times.
   return PickedMorsel{
      .morsel_size = col->size,
      .pipeline_progress = 0.0,
   };
}

std::string FuseChunkSourceDriver::id() const {
   return "FuseChunkSourceDriver";
}

void FuseChunkSourceDriver::setUpStateImpl(const ExecutionContext& context_) {
   assert(!context_.getPipe().getConsumers(*this).empty());
   cols.resize(context_.getNumThreads());
   for (size_t thread_id = 0; thread_id < context_.getNumThreads(); ++thread_id) {
      // Figure out which columns we are actually driving.
      const auto& consumer = context_.getPipe().getConsumers(*this)[0];
      const auto& driving = **consumer->getIUs().begin();
      // Get the column in the backing fuse chunk for that. The morsels
      // we will pick will go over the size of that given column in the chunk.
      cols[thread_id] = &context_.getColumn(driving, thread_id);
   }
}

std::unique_ptr<FuseChunkSourceIUProvider> FuseChunkSourceIUProvider::build(const IU& driver_iu, const IU& produced_iu) {
   return std::unique_ptr<FuseChunkSourceIUProvider>(new FuseChunkSourceIUProvider(driver_iu, produced_iu));
}

FuseChunkSourceIUProvider::FuseChunkSourceIUProvider(const IU& driver_iu, const IU& produced_iu)
   : IndexedIUProvider(nullptr, driver_iu, produced_iu) {
}

void FuseChunkSourceIUProvider::setUpStateImpl(const ExecutionContext& context) {
   for (size_t k = 0; k < context.getNumThreads(); ++k) {
      auto& state = (*states)[k];
      // Extract the raw data from which to read within the backing chunk.
      auto& col = context.getColumn(**provided_ius.begin(), k);
      state.start = col.raw_data;
      if (runtime_params.type_param) {
         // Fetch the underlying raw data from the associated runtime parameters.
         // If the value was hard-coded in the generated code already it will simply never be accessed.
         // Should only not be set in tests which manually create a FuseChunkSourceIUProvider.
         state.type_param = *reinterpret_cast<uint16_t*>(runtime_params.type_param->rawData());
      }
   }
}

std::string FuseChunkSourceIUProvider::providerName() const {
   return "FuseChunkSourceIUProvider";
}

}
