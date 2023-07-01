#ifndef INKFUSE_TABLESCANSOURCE_H
#define INKFUSE_TABLESCANSOURCE_H

#include "algebra/suboperators/IndexedIUProvider.h"
#include "algebra/suboperators/LoopDriver.h"
#include "algebra/suboperators/Suboperator.h"
#include <atomic>

/// This file contains the necessary sub-operators for reading from a base table.
namespace inkfuse {

/// Loop driver for reading a morsel from an underlying table.
struct TScanDriver final : public LoopDriver {
   static std::unique_ptr<TScanDriver> build(const RelAlgOp* source, size_t rel_size_ = 0);

   /// Pick then next set of tuples from the table scan up to the maximum chunk size.
   PickMorselResult pickMorsel(size_t thread_id) override;

   std::string id() const override;

   private:
   /// Set up the table scan driver in the respective base pipeline.
   TScanDriver(const RelAlgOp* source, size_t rel_size_);

   /// What is the size of the backing relation?
   size_t rel_size;
   /// What is the index the next morsel should start at? Atomic since
   /// multiple morsels may pick work at the same time.
   std::atomic<size_t> start_idx = 0;
   /// Was the first morsel picked already?
   bool first_picked = false;
};

/// IU provider when reading from a table scan.
struct TScanIUProvider final : public IndexedIUProvider {
   static std::unique_ptr<TScanIUProvider> build(const RelAlgOp* source, const IU& driver_iu, const IU& produced_iu, char* raw_data_ = nullptr);

   protected:
   void setUpStateImpl(const ExecutionContext& context) override;

   std::string providerName() const override;

   private:
   TScanIUProvider(const RelAlgOp* source, const IU& driver_iu, const IU& produced_iu, char* raw_data_);

   /// Pointer to the start of the backing stored column.
   char* raw_data;
};

}

#endif //INKFUSE_TABLESCANSOURCE_H
