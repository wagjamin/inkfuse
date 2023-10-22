#ifndef INKFUSE_SUBOPERATOR_H
#define INKFUSE_SUBOPERATOR_H

#include "algebra/IU.h"
#include "exec/ExecutionContext.h"
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <unordered_set>
#include <variant>
#include <vector>

namespace inkfuse {

struct CompilationContext;
struct FuseChunk;
struct RelAlgOp;
struct Pipeline;

/// A suboperator is a fragment of a central operator which has a corresponding vectorized primitive.
/// An example might be the aggregation of an IU into some aggregation state.
/// An operator can be a DAG of these fragments which can either be interpreted through the vectorized
/// primitives or fused together.
///
/// A suboperator is parametrized in two different ways:
/// - Discrete DoF: the suboperator is parametrized over a finite set of potential values. Examples might be
///                 SQL types like UInt8, Date, Timestamp, ...
/// - Infinite DoF: the suboperator is parametrized over an infinite set of potential values. Examples might be
///                 the length of a varchar type, or the offset into an aggregation state.
///
/// When we execute the query down the line, note that these parameters above are actually *not* degrees of freedom.
/// This is an important observation, at run time these are all substituted.
/// Two conclusions can be drawn from this:
///      1) In a pipeline-fusing engine, all parameters would be substitued with hard-baked values
///      2) In a vectorized engine, there would be fragments over the discrete space spanned by
///         the parameters, but the infinite degrees of freedom would be provided through function parameters.
///
/// This concept is baked through the SubstitutableParameter expression type within our IR. When generating code,
/// the actual operator is able to use both discrete and infinite degrees of freedom in a hard-coded way.
/// The substitution into either function parameters or baked-in values then happens in the lower parts of the
/// compilation stack depending on which parameters are provided to the compiler backend.
///
/// This allows switching between vectorized fragments and JIT-fused execution through the same operators,
/// parameter logic and codegen structure.
struct Suboperator {
   /// Suboperator constructor. Parametrized as described above and also fitting a certain type.
   Suboperator(const RelAlgOp* source_, std::vector<const IU*> provided_ius_, std::vector<const IU*> source_ius_)

      : source(source_), provided_ius(std::move(provided_ius_)), source_ius(std::move(source_ius_)) {}

   virtual ~Suboperator() = default;

   /// Generate initial code for this operator when IUs are requested the first time.
   /// Will usually call open on all children and make the target IUs available to the parent operator.
   virtual void open(CompilationContext& context);
   /// All downstream consumers have been closed - this operator can be closed as well.
   virtual void close(CompilationContext& context);
   /// Consume a specific IU from one of the children.
   virtual void consume(const IU& iu, CompilationContext& context){};
   /// Consume once all IUs are ready.
   virtual void consumeAllChildren(CompilationContext& context){};

   /// Is this a sink sub-operator which has no further descendants?
   bool isSink() const { return provided_ius.empty(); }
   /// Is this a source sub-operator driving execution?
   bool isSource() const { return source_ius.empty(); }

   /// Are the incoming edges to nodes of this sub-operator strong?
   /// This means that the sub-operator will retain all incoming sub-operators during a repipe.
   /// The incoming strong link is always defined on the first input IU.
   virtual bool incomingStrongLinks() const { return false; }
   /// Are the outgoing edges to nodes of this sub-operator strong?
   /// This means that the sub-operator does not have to be interpreted.
   /// This is done for scans from managed tables. Here, we have the initial IU provider
   /// that gets interpreted. We don't want to first create the loop index containing an offset
   /// into the base table within a first interpreted operation, and then read from those loop indexes.
   /// Rather, we want to fuse the IndexedIUProvider from the table scan with the IU provider directly
   /// to generate more efficient code.
   virtual bool outgoingStrongLinks() const { return false; }

   /// Set up the state needed by this operator. In an IncrementalFusion engine it's easiest to actually
   /// make this interpreted.
   /// This function has to be idempotent, i.e. multiple setUpStates should leave the operator
   /// in the same state as a single invocation.
   virtual void setUpState(const ExecutionContext& context){};
   /// Tear down the state needed by this operator.
   virtual void tearDownState(){};
   /// Get a raw pointer to the local state of a thread working on this operator.
   virtual void* accessState(size_t thread_id) const { return nullptr; };

   struct NoMoreMorsels {};
   struct PickedMorsel {
      /// How many rows the picked morsel contains.
      size_t morsel_size;
      /// Estimated progress of the pipeline [0, 1].
      double pipeline_progress;
   };
   using PickMorselResult = std::variant<NoMoreMorsels, PickedMorsel>;
   /// Either returns that there are no more morsels, or progress [0, 1]
   /// Pick a morsel of work. Only relevant for source operators.
   /// Returns the size of the picked morsel - or 0 if picking was unsuccessful.
   /// Thread safe - different threads can pick morsels in parallel.
   virtual PickMorselResult pickMorsel(size_t thread_id) {
      throw std::runtime_error("Operator does not support picking morsels");
   }

   /// Build a unique identifier for this suboperator (unique given the parameter set).
   /// This is neded to effectively use the fragment cache during vectorized interpretation.
   virtual std::string id() const = 0;
   /// Get a variable identifier which is unique to this suboperator.
   std::stringstream getVarIdentifier() const;

   /// How many ius does this suboperator depend on?
   size_t getNumSourceIUs() const { return source_ius.size(); }
   const std::vector<const IU*>& getSourceIUs() const { return source_ius; }
   const std::vector<const IU*>& getIUs() const { return provided_ius; }

   /// Properties that can influence runtime and code generation behaviour of suboperators.
   /// These allow improving the performance of the system.
   struct OptimizationProperties {
      /// Compile-time property. When set, the suboperator does not generate code
      /// for when compiled for operator-fusing code generation.
      bool ct_only_vectorized = false;
      /// Runtime property. If a chunk size preference is set, the vectorized backend
      /// will try to break a morsel into smaller chunk that respect the chunk size
      /// preference. This allows us to e.g. perform hash table lookups in a highly
      /// optimized way. We can go to smaller chunk sizes that will have better cache
      /// locality. This matters as we split prefetching and lookups into two phases.
      std::optional<size_t> rt_chunk_size_prefeference = std::nullopt;
   };
   const OptimizationProperties& getOptimizationProperties() const { return optimization_properties; };

   protected:
   /// The operator which decayed into this Suboperator.
   const RelAlgOp* source;

   /// IUs produced by this sub-operator.
   std::vector<const IU*> provided_ius;
   /// Source IUs on which this sub-operator depends. Note that these are ordered.
   /// This is important for operators like subtraction. This is important when e.g.
   /// interpreting an operator to make sure that the input columns are extracted in
   /// the right order.
   std::vector<const IU*> source_ius;

   /// Optimization properties that can be used to improve suboperator performance.
   OptimizationProperties optimization_properties;
};

/// Empty state which can be used in the templated suboperators.
struct EmptyState {};

/// Templated suboperator providing functionality required across multiple operators.
/// @tparam LocalState thread-local execution state of the operator hooked up into the inkfuse runtime.
template <class LocalState>
struct TemplatedSuboperator : public Suboperator {
   using LocalStates = std::vector<LocalState>;

   void setUpState(const ExecutionContext& context) override {
      if (states) {
         return;
      }
      states = std::make_unique<LocalStates>(context.getNumThreads());
      setUpStateImpl(context);
   };

   void tearDownState() override {
      tearDownStateImpl();
      states.reset();
   };

   void* accessState(size_t thread_id) const override {
      if (!states) {
         return nullptr;
      }
      assert(thread_id < states->size());
      return &(*states)[thread_id];
   };

   protected:
   TemplatedSuboperator(const RelAlgOp* source_, std::vector<const IU*> provided_ius_, std::vector<const IU*> source_ius_)
      : Suboperator(source_, std::move(provided_ius_), std::move(source_ius_)){};

   /// Set up the state given that the precondition that both params and state
   /// are non-empty is satisfied.
   virtual void setUpStateImpl(const ExecutionContext& context){};

   /// Tear down the state - allows to run custom cleanup logic when a query is done.
   virtual void tearDownStateImpl(){};

   /// Thread local states for the threads working on this suboperator.
   std::unique_ptr<LocalStates> states;
};

using SuboperatorArc = std::shared_ptr<Suboperator>;

}

#endif //INKFUSE_SUBOPERATOR_H
