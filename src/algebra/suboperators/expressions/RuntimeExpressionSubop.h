#ifndef INKFUSE_RUNTIMEEXPRESSIONSUBOP_H
#define INKFUSE_RUNTIMEEXPRESSIONSUBOP_H

#include "algebra/ExpressionOp.h"
#include "algebra/suboperators/RuntimeParam.h"
#include "algebra/suboperators/Suboperator.h"
#include "algebra/suboperators/properties/WithRuntimeParams.h"

namespace inkfuse {

/// Runtime state of a given runtime expression suboperator.
struct RuntimeExpressionState {
   static const char* name;

   /// Type-erased pointer to a backing value. This way we don't have to define RuntimeExpressionSubops
   /// for every value.
   void* data_erased;
};

/// Runtime parameter which gets resolved during runtime or code generation, depending on whether it is attached.
struct RuntimeExpressionParams {
   RUNTIME_PARAM(data, RuntimeExpressionState);
};

/// Runtime expression subop takes an IU argument on the right and a constant value on the left.
/// It then computes the binary expression on top of that.
struct RuntimeExpressionSubop : public TemplatedSuboperator<RuntimeExpressionState>, public WithRuntimeParams<RuntimeExpressionParams> {

   /// Constructor for directly building a smart pointer.
   static SuboperatorArc build(const RelAlgOp* source_, std::vector<const IU*> provided_ius_, std::vector<const IU*> operands_, ExpressionOp::ComputeNode::Type type_, IR::TypeArc runtime_expression_type_);
   /// Constructor.
   RuntimeExpressionSubop(const RelAlgOp* source_, std::vector<const IU*> provided_ius_, std::vector<const IU*> source_ius_, ExpressionOp::ComputeNode::Type type_, IR::TypeArc runtime_expression_type_);

   /// Register runtime structs and functions.
   static void registerRuntime();

   /// Set up the global state of this suboperator.
   void setUpStateImpl(const ExecutionContext& context) override;

   /// Consume once all IUs are ready.
   void consumeAllChildren(CompilationContext& context) override;

   std::string id() const override;

   private:
   /// Type of the runtime parameter.
   IR::TypeArc runtime_param_type;
   /// Which expression type to evaluate.
   ExpressionOp::ComputeNode::Type type;

};

} // namespace inkfuse

#endif //INKFUSE_RUNTIMEEXPRESSIONSUBOP_H
