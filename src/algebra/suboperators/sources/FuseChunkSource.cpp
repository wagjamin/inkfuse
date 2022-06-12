#include "algebra/suboperators/sources/FuseChunkSource.h"
#include "exec/ExecutionContext.h"

namespace inkfuse {

std::unique_ptr<FuseChunkSourceDriver> FuseChunkSourceDriver::build()
{
   return std::unique_ptr<FuseChunkSourceDriver>(new FuseChunkSourceDriver());
}

FuseChunkSourceDriver::FuseChunkSourceDriver(): LoopDriver<EmptyState>(nullptr)
{
}

bool FuseChunkSourceDriver::pickMorsel()
{
   assert(col);
   state->start = 0;
   state->end = col->size;
   // We can always pick a morsel for a fuse chunk. The actual execution is responsible for making
   // sure that pickMorsel for fuse chunks is invoked at the right times.
   return true;
}

std::string FuseChunkSourceDriver::id() const
{
   return "FuseChunkSourceDriver";
}

void FuseChunkSourceDriver::setUpStateImpl(const ExecutionContext& context_)
{
   assert(!context_.getPipe().getConsumers(*this).empty());
   // Figure out which columns we are actually driving.
   const auto& consumer = context_.getPipe().getConsumers(*this)[0];
   const auto& driving = **consumer->getIUs().begin();
   // Get the column in the backing fuse chunk for that. The morsels
   // we will pick will go over the size of that given column in the chunk.
   col = &context_.getColumn(driving);
}

std::unique_ptr<FuseChunkSourceIUProvider> FuseChunkSourceIUProvider::build(const IU& driver_iu, const IU& produced_iu)
{
   return std::unique_ptr<FuseChunkSourceIUProvider>(new FuseChunkSourceIUProvider(driver_iu, produced_iu));
}

FuseChunkSourceIUProvider::FuseChunkSourceIUProvider(const IU& driver_iu, const IU& produced_iu)
: IndexedIUProvider<EmptyState>(nullptr, driver_iu, produced_iu)
{
}

void FuseChunkSourceIUProvider::setUpStateImpl(const ExecutionContext& context)
{
   // Extract the raw data from which to read within the backing chunk.
   auto& col = context.getColumn(**provided_ius.begin());
   state->raw_data = col.raw_data;
}

std::string FuseChunkSourceIUProvider::providerName() const
{
   return "FuseChunkSourceIUProvider";
}

}