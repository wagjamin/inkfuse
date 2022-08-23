#ifndef INKFUSE_RUNTIMEFUNCTIONSUBOP_H
#define INKFUSE_RUNTIMEFUNCTIONSUBOP_H

#include "algebra/suboperators/Suboperator.h"

namespace inkfuse {

/// Runtime state of a given RuntimeFunctionSubop.
struct RuntimeFunctionSubopState {
   static const char* name;

   /// Backing object used as the first argument in the call.
   void* this_object;
};

/// RuntimeFunctionSubop calls a runtime function on a given object.
/// Takes an argument as input IU and computes an output IU given a runtime function.
/// An example is a hash-table lookup.
/// TODO It would be cleaner to separate the this_object out of the suboperator state.
struct RuntimeFunctionSubop : public TemplatedSuboperator<RuntimeFunctionSubopState> {
   static const bool hasConsume = false;
   static const bool hasConsumeAll = true;

   /// Build a hash table lookup function.
   static std::unique_ptr<RuntimeFunctionSubop> htLookup(const RelAlgOp* source, const IU& hashes_, void* hash_table_ = nullptr);

   static void registerRuntime();

   void consumeAllChildren(CompilationContext& context) override;

   void setUpStateImpl(const ExecutionContext& context) override;

   std::string id() const override;

   protected:
   RuntimeFunctionSubop(const RelAlgOp* source, std::string fct_name_, const IU& arg, void* this_object_);

   /// The function name.
   std::string fct_name;
   /// The object passed as first argument to the runtime function.
   void* this_object;
   /// Input IU passed as the first argument.
   const IU& arg;
   /// The generated output IU containing the initial iterators.
   const IU produced;
};


}

#endif //INKFUSE_RUNTIMEFUNCTIONSUBOP_H
