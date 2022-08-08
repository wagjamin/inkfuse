#ifndef INKFUSE_AGGSTATE_H
#define INKFUSE_AGGSTATE_H

#include "codegen/Expression.h"

namespace inkfuse {

struct KeyPackingRuntimeParams;
struct IU;
namespace IR {
struct FunctionBuilder;
}

/// Aggregation state computation used by an AggregatorSubop.
struct AggState {

   /// Virtual base destructor.
   virtual ~AggState() = default;

   AggState(IR::TypeArc type_);

   /// Initialize the aggregate state. Materialized the first value into the
   /// aggregation state. This is important as for a minimum aggregation we
   /// might not be able to discern the initial state after the group was allocated
   /// with an exiting value.
   virtual void initState(IR::FunctionBuilder& builder, IR::ExprPtr ptr, IR::ExprPtr val) const = 0;

   /// Update the aggregate state.
   virtual void updateState(IR::FunctionBuilder& builder, IR::ExprPtr ptr, IR::ExprPtr val) const = 0;

   /// Get the size of the backing aggregate state.
   virtual size_t getStateSize() const = 0;

   virtual std::string id() const = 0;

   protected:
   IR::TypeArc type;
};

}

#endif //INKFUSE_AGGSTATE_H
