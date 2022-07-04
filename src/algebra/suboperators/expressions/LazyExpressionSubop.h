#ifndef INKFUSE_LAZYEXPRESSIONSUBOP_H
#define INKFUSE_LAZYEXPRESSIONSUBOP_H

#include "algebra/ExpressionOp.h"
#include "algebra/suboperators/LazyParam.h"
#include "algebra/suboperators/Suboperator.h"
#include "algebra/suboperators/properties/WithLazyParams.h"

namespace inkfuse {

/// Runtime state of a given lazy expression suboperator.
struct LazyExpressionSubopState {
   static const char* name;

   /// Type-erased pointer to a backing value. This way we don't have to define LazyExpressionSubops
   /// for every value.
   void* data_erased;
};

/// Lazy parameter which gets resolved during runtime or code generation, depending on whether it is attached.
struct LazyExpressionParams {
   LAZY_PARAM(data, LazyExpressionSubopState);
};

/// Lazy expression subop takes an IU argument on the left and a constant value on the right.
/// It then computes the binary expression on top of that.
struct LazyExpressionSubop : public TemplatedSuboperator<LazyExpressionSubopState, EmptyState>, WithLazyParams<LazyExpressionParams> {

   /// Constructor for directly building a smart pointer.
   static SuboperatorArc build(const RelAlgOp* source_, std::vector<const IU*> provided_ius_, std::vector<const IU*> operands_, ExpressionOp::ComputeNode::Type type_, IR::TypeArc lazy_expression_type_);
   /// Constructor.
   LazyExpressionSubop(const RelAlgOp* source_, std::vector<const IU*> provided_ius_, std::vector<const IU*> source_ius_, ExpressionOp::ComputeNode::Type type_, IR::TypeArc lazy_expression_type_);

   /// Register runtime structs and functions.
   static void registerRuntime();

   /// Set up the global state of this suboperator.
   void setUpStateImpl(const ExecutionContext& context) override;

   /// Consume once all IUs are ready.
   void consumeAllChildren(CompilationContext& context) override;

   std::string id() const override;

   private:
   /// Type of the lazy parameter.
   IR::TypeArc lazy_param_type;
   /// Which expression type to evaluate.
   ExpressionOp::ComputeNode::Type type;

};

} // namespace inkfuse

#endif //INKFUSE_LAZYEXPRESSIONSUBOP_H
