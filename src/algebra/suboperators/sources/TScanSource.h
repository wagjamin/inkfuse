#ifndef INKFUSE_TSCANSOURCE_H
#define INKFUSE_TSCANSOURCE_H

#include "algebra/suboperators/Suboperator.h"
#include "storage/Relation.h"

/// This file contains the necessary sub-operators for reading from a base table.
namespace inkfuse {

/// Runtime state of a given TScan.
struct TScanDriverState {
   static constexpr char* name = "TScanDriverState";
   /// Index of the first tuple to read (inclusive).
   uint64_t start;
   /// Index of the last tuple to read (exclusive).
   uint64_t end;
};

/// Loop driver for reading a morsel from an underlying table.
struct TScanDriver : public Suboperator {

   std::unique_ptr<TScanDriver> build(const RelAlgOp* source, const StoredRelation& rel);

   /// Source - only produce is relevant and creates the respective loop driving execution.
   virtual void produce(CompilationContext& context) const;

   /// The LoopDriver is a source operator which picks morsel ranges from the backing storage.
   bool isSource() override { return true; }
   /// Pick then next set of tuples from the table scan up to the maximum chunk size.
   bool pickMorsel() override;

   void setUpState() override;
   void tearDownState() override;
   void * accessState() const override;

   std::string id() const override { return "tscandriver"; };

   /// Register runtime structs and functions.
   static void registerRuntime();

   private:
   /// Set up the table scan driver in the respective base pipeline.
   TScanDriver(const RelAlgOp* source, const StoredRelation& rel);

   /// Loop counter IU provided by the TScanDriver.
   IU loop_driver_iu;
   /// Was the first morsel picked already?
   bool first_picked = false;
   /// Size of the backing relation.
   size_t rel_size;
   /// Runtime state.
   std::unique_ptr<TScanDriverState> state;
};

struct TScanIUProviderState {
   static constexpr char* name = "TScanIUProviderState";

   /// Pointer to the raw data of the underlying column.
   void * raw_data;
};

struct TScanIUProviderParams {

   /// Type being read by the table scan.
   IR::TypeArc type;
};

/// IU provider when reading from a table scan.
struct TSCanIUProvider : public Suboperator {

   /// Register runtime structs and functions.
   static void registerRuntime();

   private:
   TSCanIUProvider(const RelAlgOp* source, IURef driver_iu, std::string_view column_name);




};

}

#endif //INKFUSE_TSCANSOURCE_H
