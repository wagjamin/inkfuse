#ifndef INKFUSE_HTLOOKUPSUBOP_H
#define INKFUSE_HTLOOKUPSUBOP_H

#include "algebra/suboperators/Suboperator.h"

namespace inkfuse {

struct HashTable;

/// Runtime state of a given HtLookupSubop.
struct HtLookupSubopState {
   static const char* name;

   /// Backing hash table to be used in the lookup.
   void* hash_table;
};

/// Simple sub-operator performing a hash-table lookup.
/// Takes a hash as input IU and computes a hash table location as output.
/// Signature: HtLookupSubop(uint64_t) -> void*
/// TODO It would be cleaner to separate the hash table out of the suboperator state.
struct HtLookupSubop : public TemplatedSuboperator<HtLookupSubopState> {

   static std::unique_ptr<HtLookupSubop> build(const RelAlgOp* source, const IU& hashes_, HashTable* hash_table_ = nullptr);

   static void registerRuntime();

   void consumeAllChildren(CompilationContext& context) override;

   void setUpStateImpl(const ExecutionContext& context) override;

   std::string id() const override;

   private:
   HtLookupSubop(const RelAlgOp* source, const IU& hashes_, HashTable* hash_table_);

   /// The hash table into which lookups should be performed.
   HashTable* hash_table;
   /// IU containing the hashes.
   const IU& hashes;
   /// The generated output IU containing the initial iterators.
   const IU produced;
};


}

#endif //INKFUSE_HTLOOKUPSUBOP_H
