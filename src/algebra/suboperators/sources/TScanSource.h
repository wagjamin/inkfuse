#ifndef INKFUSE_TSCANSOURCE_H
#define INKFUSE_TSCANSOURCE_H

#include "algebra/suboperators/Suboperator.h"
#include "algebra/suboperators/IndexedIUProvider.h"
#include "codegen/IRBuilder.h"

/// This file contains the necessary sub-operators for reading from a base table.
namespace inkfuse {

/// Runtime state of a given TScan.
struct TScanDriverState {
   static const char* name;

   /// Index of the first tuple to read (inclusive).
   uint64_t start;
   /// Index of the last tuple to read (exclusive).
   uint64_t end;
};

/// Runtime parameters which are not needed for code generation of the respective operator.
struct TScanDriverRuntimeParams {
   /// Size of the backing relation.
   size_t rel_size;
};

/// Loop driver for reading a morsel from an underlying table.
struct TScanDriver : public TemplatedSuboperator<TScanDriverState, TScanDriverRuntimeParams> {
   static std::unique_ptr<TScanDriver> build(const RelAlgOp* source);

   void rescopePipeline(Pipeline& pipe) override;

   /// Source - only open and close are relevant and create the respective loop driving execution.
   void open(CompilationContext& context) override;
   void close(CompilationContext& context) override;

   /// The LoopDriver is a source operator which picks morsel ranges from the backing storage.
   bool isSource() const override { return true; }
   /// Pick then next set of tuples from the table scan up to the maximum chunk size.
   bool pickMorsel() override;

   std::string id() const override { return "tscandriver"; };

   /// Register runtime structs and functions.
   static void registerRuntime();

   private:
   /// Set up the table scan driver in the respective base pipeline.
   TScanDriver(const RelAlgOp* source);

   /// Loop counter IU provided by the TScanDriver.
   IU loop_driver_iu;
   /// Was the first morsel picked already?
   bool first_picked = false;
   /// If code generation is in progress - the generated while loop whose scope
   /// has to be closed down the line.
   std::optional<IR::While> opt_while;
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

#endif //INKFUSE_TSCANSOURCE_H
