#ifndef INKFUSE_TSCANSOURCE_H
#define INKFUSE_TSCANSOURCE_H

#include "algebra/suboperators/Suboperator.h"
#include "storage/Relation.h"

// TODO Make attachRuntimeParams and ternary parameter structure
//      a template class.

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
struct TScanDriver : public Suboperator {
   std::unique_ptr<TScanDriver> build(const RelAlgOp* source);

   void attachRuntimeParams(TScanDriverRuntimeParams runtime_params_);

   /// Source - only produce is relevant and creates the respective loop driving execution.
   void produce(CompilationContext& context) const override;

   /// The LoopDriver is a source operator which picks morsel ranges from the backing storage.
   bool isSource() override { return true; }
   /// Pick then next set of tuples from the table scan up to the maximum chunk size.
   bool pickMorsel() override;

   void setUpState() override;
   void tearDownState() override;
   void* accessState() const override;

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
   /// Runtime state.
   std::unique_ptr<TScanDriverState> state;
   /// Optional runtime parameters.
   std::optional<TScanDriverRuntimeParams> runtime_params;
};

struct TScanIUProviderState {
   static const char* name;

   /// Pointer to the raw data of the underlying column.
   void* raw_data;
};

struct TScanIUProviderParams {
   /// Type being read by the table scan.
   IR::TypeArc type;
};

/// Runtime parameters which are not needed for code generation of the respective operator.
struct TScanIUProviderRuntimeParams {
   /// Raw data.
   void* raw_data;
   /// Actual name of the column to be read.
   std::string col_name;
};

/// IU provider when reading from a table scan.
struct TSCanIUProvider : public Suboperator {
   static std::unique_ptr<TSCanIUProvider> build(const RelAlgOp* source, IURef driver_iu, IURef produced_iu, std::string_view column_name, const StoredRelation& rel);

   /// Attach runtime parameters for this sub-operator.
   void attachRuntimeParams(TScanIUProviderRuntimeParams runtime_params_);

   void consume(const IU& iu, CompilationContext& context) const override;

   void setUpState() override;
   void tearDownState() override;
   void* accessState() const override;

   std::string id() const override;

   /// Register runtime structs and functions.
   static void registerRuntime();

   private:
   TSCanIUProvider(const RelAlgOp* source, IURef driver_iu, IURef produced_iu);

   /// Static parameters of the operator.
   TScanIUProviderParams params;
   /// Runtime state.
   std::unique_ptr<TScanIUProviderState> state;
   /// Optional runtime parameters.
   std::optional<TScanIUProviderRuntimeParams> runtime_params;
};

}

#endif //INKFUSE_TSCANSOURCE_H
