#ifndef INKFUSE_KEYEQUALITYEXPRESSION_H
#define INKFUSE_KEYEQUALITYEXPRESSION_H

#include "algebra/suboperators/RuntimeParam.h"
#include "algebra/suboperators/Suboperator.h"
#include "algebra/suboperators/properties/WithRuntimeParams.h"

namespace inkfuse {

/// Runtime state of a given runtime key expression suboperator.
struct RuntimeKeyExpressionState {
   static const char* name;

   /// Pointer offset within the given key.
   uint16_t offset;
};

/// Runtime parameter which gets resolved during runtime or code generation, depending on whether it is attached.
struct RuntimeKeyExpressionParams {
   RUNTIME_PARAM(offset, RuntimeKeyExpressionState);
};

/// RuntimeKeyExpressionSubop is similar to a regular RuntimeExpression. However, it takes a runtime parameter
/// describing a pointer offset. It then does an equality check on a given input IU and a pointer IU at the given offset.
/// Why is this needed? This is used to perfom key-equality checks within aggregation and join hash tables.
/// Signature: RuntimeKeyExpressionSubop<Type>(Type elem, void* compare) -> bool
struct RuntimeKeyExpressionSubop : public TemplatedSuboperator<RuntimeKeyExpressionState>, public WithRuntimeParams<RuntimeKeyExpressionParams> {
   /// Constructor for directly building a smart pointer.
   static SuboperatorArc build(const RelAlgOp* source_, const IU& provided_iu_, const IU& source_iu_elem, const IU& source_iu_ptr);

   /// Register runtime structs and functions.
   static void registerRuntime();

   /// Set up the global state of this suboperator.
   void setUpStateImpl(const ExecutionContext& context) override;

   /// Consume once all IUs are ready.
   void consumeAllChildren(CompilationContext& context) override;

   std::string id() const override;

   private:
   /// Constructor.
   RuntimeKeyExpressionSubop(const RelAlgOp* source_, const IU& provided_ius_, const IU& source_iu_elem, const IU& source_iu_ptr);
};

}

#endif //INKFUSE_KEYEQUALITYEXPRESSION_H
