#ifndef INKFUSE_HASHTABLESOURCE_H
#define INKFUSE_HASHTABLESOURCE_H

#include "algebra/CompilationContext.h"
#include "algebra/RelAlgOp.h"
#include "algebra/suboperators/Suboperator.h"
#include "codegen/IRBuilder.h"

namespace inkfuse {

/// Runtime state of the hash table source.
struct HashTableSourceState {
   static const char* name;

   /// Backing hash table from which we want to read.
   void* hash_table;
   /// Iterator pointer at which to start.
   char* it_ptr_start;
   /// Current iterator index.
   uint64_t it_idx_start;
   /// Iterator pointer at which to end.
   char* it_ptr_end;
   /// Iterator end index.
   uint64_t it_idx_end;
};

/// The HashTableSouce allows reading from an underlying hash table. It returns char pointers to the
/// hash table payloads.
/// Picks morsels until all hash table payloads were produced. Within a single morsel, produces at most
/// DEFAULT_CHUNK_SIZE elements.
struct HashTableSource : public TemplatedSuboperator<HashTableSourceState> {

   static SuboperatorArc build(const RelAlgOp* source, const IU& produced_iu, HashTableSimpleKey* hash_table_);

   /// Keep running as long as we have cells to read from in the backing hash table.
   bool pickMorsel() override;

   void open(CompilationContext& context) override;

   void close(CompilationContext& context) override;

   std::string id() const override { return "hash_table_source"; };

   /// Register the HashTableSourceState in the global inkfuse runtime.
   static void registerRuntime();

   protected:
   HashTableSource(const RelAlgOp* source, const IU& produced_iu, HashTableSimpleKey* hash_table_);

   void setUpStateImpl(const ExecutionContext& context) override;

private:
   /// In-flight while loop being generated between calls to open() and close().
   std::optional<IR::While> opt_while;
   /// Global state copied into the function preamble.
   IR::Stmt* decl_ht;
   IR::Stmt* decl_it_idx;
   /// The hash table we are reading from.
   // TODO(benjamin) it's not clean to have the actual object as a suboperator member.
   HashTableSimpleKey* hash_table;
};

}

#endif //INKFUSE_HASHTABLESOURCE_H
