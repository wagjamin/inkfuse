#ifndef INKFUSE_AGGSTATE_H
#define INKFUSE_AGGSTATE_H

#include "codegen/Type.h"

namespace inkfuse {

struct IU;
namespace IR {
struct FunctionBuilder;
struct Stmt;
}

/// Aggregation state computation used by an AggregatorSubop.
struct AggState {

   /// Virtual base destructor.
   virtual ~AggState() = default;

   AggState(IR::TypeArc type_);

   /// Does this aggregate need extra state initialization, or can it it work on
   /// zero initialized memory?
   virtual bool needsStateInit() const = 0;

   /// Initialize the aggregate state. Materialized the first value into the
   /// aggregation state. This is important as for a minimum aggregation we
   /// might not be able to discern the initial state after the group was allocated
   /// with an exiting value.
   virtual void initState(IR::FunctionBuilder& builder, const IR::Stmt& ptr, const IR::Stmt& val) const = 0;

   /// Update the aggregate state.
   virtual void updateState(IR::FunctionBuilder& builder, const IR::Stmt& ptr, const IR::Stmt& val) const = 0;

   /// Get the size of the backing aggregate state.
   virtual size_t getStateSize() const = 0;

   virtual std::string id() const = 0;

   protected:
   /// Type on which the aggregation state update is computed.
   IR::TypeArc type;
};

struct ZeroInitializedAggState : public AggState {

   ZeroInitializedAggState(IR::TypeArc type_) : AggState(std::move(type_)) {
   }

   bool needsStateInit() const override final { return false; };

   void initState(IR::FunctionBuilder& builder, const IR::Stmt& ptr, const IR::Stmt& val) const override final {
      throw std::runtime_error(id() + " does not support state initialization.");
   };

};

using AggStatePtr = std::unique_ptr<AggState>;

}

#endif //INKFUSE_AGGSTATE_H
