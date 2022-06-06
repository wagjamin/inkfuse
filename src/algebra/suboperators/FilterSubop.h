#ifndef INKFUSE_FILTERSUBOP_H
#define INKFUSE_FILTERSUBOP_H

#include "algebra/Filter.h"
#include "algebra/IUScope.h"
#include "algebra/suboperators/Suboperator.h"
#include "codegen/IRBuilder.h"

namespace inkfuse {

/// The FilterSubop is created by the Filter operator. It performs no computation but only
/// rescopes the pipeline appropriately.
/// The actual evaluation of the predicate needs to be done by a suitable ExpressionSubop before
/// the filter.
struct FilterSubop : public TemplatedSuboperator<EmptyState, EmptyState> {
   /// Create a new filter sub-operator.
   /// @param provided_ius_ all input ius which will be needed further up the filter.
   ///                      The filter is created as both consumer of these IUs in the old scope,
   ///                      and as provider of the IUs in the new scope.
   FilterSubop(const RelAlgOp* source_, std::vector<const IU*> provided_ius_, const IU& filter_iu_);

   /// A FilterSubop has RescopeRetain semantics and installs the filter iu as the selection vector in the new scope.
   std::pair<ScopingBehaviour, const IU*> scopingBehaviour() const override;

   std::string id() const override;

   /// We can only generate the actual filter code once all source ius were created.
   void consumeAllChildren(CompilationContext& context) override;
   /// On close we can terminate the if block we have created for downstream consumers.
   void close(CompilationContext& context) override;

   private:
   // IU on which we filter.
   const IU& filter_iu;
   /// In-flight if statement being generated.
   std::optional<IR::If> opt_if;
};

}

#endif //INKFUSE_FILTERSUBOP_H
