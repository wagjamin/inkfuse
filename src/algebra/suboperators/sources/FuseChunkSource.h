#ifndef INKFUSE_FCHUNKSOURCE_H
#define INKFUSE_FCHUNKSOURCE_H

#include "algebra/suboperators/Suboperator.h"
#include "algebra/suboperators/IndexedIUProvider.h"
#include "algebra/suboperators/LoopDriver.h"

namespace inkfuse {

struct FuseChunkSourceDriver final : public LoopDriver {

   static std::unique_ptr<FuseChunkSourceDriver> build();

   /// Pick then next set of tuples from the table scan up to the maximum chunk size.
   PickMorselResult pickMorsel() override;

   std::string id() const override;

   /// Set up the state given that the precondition that both params and state
   /// are non-empty is satisfied.
   void setUpStateImpl(const ExecutionContext& context) override;

   private:
   /// Set up the table scan driver in the respective base pipeline.
   FuseChunkSourceDriver();
   Column* col = nullptr;
};

struct FuseChunkSourceIUProvider final : public IndexedIUProvider {
   static std::unique_ptr<FuseChunkSourceIUProvider> build(const IU& driver_iu, const IU& produced_iu);

   protected:
   void setUpStateImpl(const ExecutionContext& context) override;

   std::string providerName() const override;

   private:
   FuseChunkSourceIUProvider(const IU& driver_iu, const IU& produced_iu);
};

}

#endif //INKFUSE_FCHUNKSOURCE_H
