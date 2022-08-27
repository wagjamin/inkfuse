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
/// TODO(benjamin) this is not very tidy code:
///   - It would be cleaner to separate the this_object out of the suboperator state.
///   - Additionally we should clean up the argument/return type mess. For example the implicit derefing is really nasty.
struct RuntimeFunctionSubop : public TemplatedSuboperator<RuntimeFunctionSubopState> {
   static const bool hasConsume = false;
   static const bool hasConsumeAll = true;

   /// Build a hash table lookup function.
   static std::unique_ptr<RuntimeFunctionSubop> htLookup(const RelAlgOp* source, const IU& pointers_, const IU& key_, void* hash_table_ = nullptr);

   /// Build a hash table lookup or insert function.
   static std::unique_ptr<RuntimeFunctionSubop> htLookupOrInsert(const RelAlgOp* source, const IU& pointers_, const IU& key_, void* hash_table_ = nullptr);

   static void registerRuntime();

   void consumeAllChildren(CompilationContext& context) override;

   void setUpStateImpl(const ExecutionContext& context) override;

   std::string id() const override;

   protected:
   RuntimeFunctionSubop(
      const RelAlgOp* source,
      std::string fct_name_,
      std::vector<const IU*> in_ius_,
      std::vector<const IU*> out_ius_,
      std::vector<const IU*> args_,
      std::vector<bool> ref_,
      const IU* out_,
      void* this_object_);

   /// The function name.
   std::string fct_name;
   /// The object passed as first argument to the runtime function.
   void* this_object;
   /// The IUs used as arguments.
   std::vector<const IU*> args;
   /// Reference annotations - which of the arguments need to be referenced before being passed to the function.
   std::vector<bool> ref;
   /// Optional output IU.
   const IU* out;
};


}

#endif //INKFUSE_RUNTIMEFUNCTIONSUBOP_H
