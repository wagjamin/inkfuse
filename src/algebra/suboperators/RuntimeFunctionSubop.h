#ifndef INKFUSE_RUNTIMEFUNCTIONSUBOP_H
#define INKFUSE_RUNTIMEFUNCTIONSUBOP_H

#include "algebra/suboperators/Suboperator.h"
#include "exec/DeferredState.h"

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

   /// Materialize a tuple in a dense tuplebuffer.
   static std::unique_ptr<RuntimeFunctionSubop> materializeTuple(const RelAlgOp* source, const IU& ptr_out_, std::vector<const IU*> inputs, DefferredStateInitializer* state_init_ = nullptr);

   /// Build an insert function for a hash table.
   static std::unique_ptr<RuntimeFunctionSubop> htInsert(const RelAlgOp* source, const IU* pointers_, const IU& key_, std::vector<const IU*> pseudo_ius_, DefferredStateInitializer* state_init_ = nullptr);

   /// Hash a key with the hash table's hash function and prefetch the corresponding slot.
   template <class HashTable>
   static std::unique_ptr<RuntimeFunctionSubop> htHashAndPrefetch(const RelAlgOp* source, const IU& hash_, const IU& key_, std::vector<const IU*> pseudo_ius_, std::optional<size_t> key_width, DefferredStateInitializer* state_init_ = nullptr) {
      std::string fct_name = "ht_" + HashTable::ID + "_compute_hash_and_prefetch";
      if (key_width && *key_width == 4) {
         // Call into four byte specialization.
         fct_name += "_fixed_4";
      } else if (key_width && *key_width == 8) {
         // Call into eight byte specialization.
         fct_name += "_fixed_8";
      }
      std::vector<const IU*> in_ius{&key_};
      for (auto pseudo : pseudo_ius_) {
         // Pseudo IUs are used as input IUs in the backing graph, but do not influence arguments.
         in_ius.push_back(pseudo);
      }
      std::vector<bool> ref{key_.type->id() != "ByteArray" && key_.type->id() != "Ptr_Char"};
      std::vector<const IU*> out_ius_{&hash_};
      std::vector<const IU*> args{&key_};
      const IU* out = &hash_;
      return std::unique_ptr<RuntimeFunctionSubop>(
         new RuntimeFunctionSubop(
            source,
            state_init_,
            std::move(fct_name),
            std::move(in_ius),
            std::move(out_ius_),
            std::move(args),
            std::move(ref),
            out));
   }

   /// Build a hash table lookup function.
   template <class HashTable, bool disable_slot>
   static std::unique_ptr<RuntimeFunctionSubop> htLookupWithHash(const RelAlgOp* source, const IU& pointers_, const IU& key_, const IU& hash_, const IU* prefetch_pseudo_, DefferredStateInitializer* state_init_ = nullptr) {
      std::string fct_name = "ht_" + HashTable::ID + "_lookup_with_hash";
      if constexpr (disable_slot) {
         fct_name += "_disable";
      }
      std::vector<const IU*> in_ius{&key_, &hash_};
      if (prefetch_pseudo_) {
         in_ius.push_back(prefetch_pseudo_);
      }
      std::vector<bool> ref{key_.type->id() != "ByteArray" && key_.type->id() != "Ptr_Char", false};
      std::vector<const IU*> out_ius_{&pointers_};
      std::vector<const IU*> args{&key_, &hash_};
      const IU* out = &pointers_;
      return std::unique_ptr<RuntimeFunctionSubop>(
         new RuntimeFunctionSubop(
            source,
            state_init_,
            std::move(fct_name),
            std::move(in_ius),
            std::move(out_ius_),
            std::move(args),
            std::move(ref),
            out));
   }

   /// Build a hash table lookup function.
   template <class HashTable>
   static std::unique_ptr<RuntimeFunctionSubop> htLookup(const RelAlgOp* source, const IU& pointers_, const IU& key_, std::vector<const IU*> pseudo_ius_, DefferredStateInitializer* state_init_ = nullptr) {
      std::string fct_name = "ht_" + HashTable::ID + "_lookup";
      std::vector<const IU*> in_ius{&key_};
      for (auto pseudo : pseudo_ius_) {
         // Pseudo IUs are used as input IUs in the backing graph, but do not influence arguments.
         in_ius.push_back(pseudo);
      }
      std::vector<bool> ref{key_.type->id() != "ByteArray" && key_.type->id() != "Ptr_Char"};
      std::vector<const IU*> out_ius_{&pointers_};
      std::vector<const IU*> args{&key_};
      const IU* out = &pointers_;
      return std::unique_ptr<RuntimeFunctionSubop>(
         new RuntimeFunctionSubop(
            source,
            state_init_,
            std::move(fct_name),
            std::move(in_ius),
            std::move(out_ius_),
            std::move(args),
            std::move(ref),
            out));
   }

   /// Build a hash table lookup or insert function.
   template <class HashTable>
   static std::unique_ptr<RuntimeFunctionSubop> htLookupOrInsert(const RelAlgOp* source, const IU* pointers_, const IU& key_, std::vector<const IU*> pseudo_ius_, DefferredStateInitializer* state_init_ = nullptr) {
      std::string fct_name = "ht_" + HashTable::ID + "_lookup_or_insert";
      std::vector<const IU*> in_ius{&key_};
      for (auto pseudo : pseudo_ius_) {
         // Pseudo IUs are used as input IUs in the backing graph, but do not influence arguments.
         in_ius.push_back(pseudo);
      }
      // The argument needs to be referenced if we directly use a non-packed IU as argument.
      std::vector<bool> ref{key_.type->id() != "ByteArray" && key_.type->id() != "Ptr_Char"};
      std::vector<const IU*> out_ius_;
      if (pointers_) {
         out_ius_.push_back(pointers_);
      }
      std::vector<const IU*> args{&key_};
      return std::unique_ptr<RuntimeFunctionSubop>(
         new RuntimeFunctionSubop(
            source,
            state_init_,
            std::move(fct_name),
            std::move(in_ius),
            std::move(out_ius_),
            std::move(args),
            std::move(ref),
            pointers_));
   }

   /// Build a lookup function for a hash table with a 0-byte key.
   static std::unique_ptr<RuntimeFunctionSubop> htNoKeyLookup(const RelAlgOp* source, const IU& pointers_, const IU& input_dependency, DefferredStateInitializer* state_init_ = nullptr);

   static void registerRuntime();

   void consumeAllChildren(CompilationContext& context) override;

   void setUpStateImpl(const ExecutionContext& context) override;

   std::string id() const override;

   std::string fname() const { return fct_name; }

   protected:
   RuntimeFunctionSubop(
      const RelAlgOp* source,
      DefferredStateInitializer* state_init_,
      std::string fct_name_,
      std::vector<const IU*>
         in_ius_,
      std::vector<const IU*>
         out_ius_,
      std::vector<const IU*>
         args_,
      std::vector<bool>
         ref_,
      const IU* out_);

   /// The deferred state initializer used to set up runtime state.
   DefferredStateInitializer* state_init;
   /// The function name.
   std::string fct_name;
   /// The IUs used as arguments.
   std::vector<const IU*> args;
   /// Reference annotations - which of the arguments need to be referenced before being passed to the function.
   std::vector<bool> ref;
   /// Optional output IU.
   const IU* out;
};
}

#endif //INKFUSE_RUNTIMEFUNCTIONSUBOP_H
