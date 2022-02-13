#ifndef INKFUSE_TABLESCANSOURCE_H
#define INKFUSE_TABLESCANSOURCE_H

#include "algebra/suboperators/Suboperator.h"
#include "algebra/suboperators/IndexedIUProvider.h"
#include "algebra/suboperators/LoopDriver.h"
#include "codegen/IRBuilder.h"

/// This file contains the necessary sub-operators for reading from a base table.
namespace inkfuse {

/// Runtime parameters which are not needed for code generation of the respective operator.
struct TScanDriverRuntimeParams {
   /// Size of the backing relation.
   size_t rel_size;
};

/// Loop driver for reading a morsel from an underlying table.
struct TScanDriver final : public LoopDriver<TScanDriverRuntimeParams> {
   static std::unique_ptr<TScanDriver> build(const RelAlgOp* source);

   /// Pick then next set of tuples from the table scan up to the maximum chunk size.
   bool pickMorsel() override;

   std::string id() const override;

   private:
   /// Set up the table scan driver in the respective base pipeline.
   TScanDriver(const RelAlgOp* source);

   /// Was the first morsel picked already?
   bool first_picked = false;
};

/// Runtime parameters which are needed to get a table scan IU provider running.
struct TScanIUProviderRuntimeParams {
   /// Raw data for the backing relation.
   void* raw_data;
};

/// IU provider when reading from a table scan.
struct TScanIUProvider final : public IndexedIUProvider<TScanIUProviderRuntimeParams> {
   static std::unique_ptr<TScanIUProvider> build(const RelAlgOp* source, const IU& driver_iu, const IU& produced_iu);

   protected:
   void setUpStateImpl(const ExecutionContext& context) override;

   std::string providerName() const override;

   private:
   TScanIUProvider(const RelAlgOp* source, const IU& driver_iu, const IU& produced_iu);
};

}

#endif //INKFUSE_TABLESCANSOURCE_H
