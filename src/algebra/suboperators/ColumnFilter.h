#ifndef INKFUSE_COLUMNCOMPACTOR_H
#define INKFUSE_COLUMNCOMPACTOR_H

#include "algebra/Filter.h"
#include "algebra/suboperators/Suboperator.h"
#include "codegen/IRBuilder.h"

/// This file contains all the sub-operators required to get filters working in ink-fuse.
/// Filters are split into two sub-operators. An initial, single ColumnFilterScope sub-operator
/// performs scoping logic for the successive re-definition.
/// Afterwards, a ColumnFilterLogic sub-operator is used to perfom the actual logic of re-defining.
/// Edges between the operators are strong. This allows us to split filter interpretation into a row
/// of vectorized primitive invocations.
/// A ColumnFilterScope and a ColumnFilterLogic operator are connected through a void-typed pseudo IU
/// that is never defined.
namespace inkfuse {

/// Scoping operator building the if statement needed in a filter.
struct ColumnFilterScope : public TemplatedSuboperator<EmptyState> {

   /// Set up a ColumnFilterScope that consumes the filter IU and defines a new pseudo IU.
   static SuboperatorArc build(const RelAlgOp* source_, const IU& filter_iu_, const IU& pseudo);

   /// A ColumnFilterScope sub-operator does not have to be interpreted.
   /// Rather, it will get warped into the primitives of the successive ColumnFilterLogic operator.
   bool outgoingStrongLinks() const override { return true; }

   /// We can only generate the actual filter scoping logic once all source ius were created.
   void consumeAllChildren(CompilationContext& context) override;
   /// On close we can terminate the if block we have created for downstream consumers.
   void close(CompilationContext& context) override;

   std::string id() const override;

   private:
   ColumnFilterScope(const RelAlgOp* source_, const IU& filter_iu_, const IU& pseudo);

   /// In-flight if statement being generated.
   std::optional<IR::If> opt_if;
};

/// Logic operator redefining the IU as a copy of the old one.
struct ColumnFilterLogic : public TemplatedSuboperator<EmptyState> {

   /// Set up a ColumnFilterLogic that consumes the ColumnFilterScope pseudo IU and redefines the incoming one.
   static SuboperatorArc build(const RelAlgOp* source_, const IU& pseudo, const IU& incoming, const IU& redefined);

   /// A ColumnFilterLogic sub-operator will wrap the incoming ColumnFilterScope
   /// operator into its vectorized primitive.
   bool incomingStrongLinks() const override { return true; }

   /// Redefines the backing column.
   void consumeAllChildren(CompilationContext& context) override;

   std::string id() const override;

   private:
   ColumnFilterLogic(const RelAlgOp* source_, const IU& pseudo, const IU& incoming, const IU& redefined);

};

}

#endif //INKFUSE_COLUMNCOMPACTOR_H
