#ifndef INKFUSE_SUBOPERATOR_H
#define INKFUSE_SUBOPERATOR_H

#include "algebra/IU.h"
#include <sstream>
#include <memory>
#include <set>
#include <vector>

namespace inkfuse {

struct CompilationContext;
struct FuseChunk;
struct RelAlgOp;
struct Pipeline;

/// A scoped IU represents a specific instance of the IU within the larger pipeline.
struct IUScoped {
   IUScoped(const IU& iu_, size_t scope_id): iu(iu_), scope_id(scope_id) {}

   /// The IU being referenced.
   const IU& iu;
   /// The scope id in which the IU is being referenced.
   const size_t scope_id;
};

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
   Suboperator(const RelAlgOp* source_, std::set<const IU*> provided_ius_, std::set<const IU*> source_ius_)

      : source(source_), provided_ius(std::move(provided_ius_)), source_ius(std::move(source_ius_)) {}

   virtual ~Suboperator() = default;

   /// Rescope a pipeline based on this suboperator.
   virtual void rescopePipeline(Pipeline& pipe) {};

   /// Generate initial code for this operator when IUs are requested the first time.
   /// Will usually call open on all children and make the target IUs available to the parent operator.
   virtual void open(CompilationContext& context);
   /// All downstream consumers have been closed - this operator can be closed as well.
   virtual void close(CompilationContext& context);
   /// Consume a specific IU from one of the children.
   virtual void consume(const IU& iu, CompilationContext& context) {};
   /// Consume once all IUs are ready.
   virtual void consumeAllChildren(CompilationContext& context) {};

   /// Is this a sink sub-operator which has no further descendants?
   virtual bool isSink() const { return false; }
   /// Is this a source sub-operator driving execution?
   virtual bool isSource() const { return false; }

   /// Set up the state needed by this operator. In an IncrementalFusion engine it's easiest to actually
   /// make this interpreted.
   virtual void setUpState(){};
   /// Tear down the state needed by this operator.
   virtual void tearDownState(){};
   /// Get a raw pointer to the state of this operator.
   virtual void* accessState() const { return nullptr; };
   /// Pick a morsel of work. Only relevant for source operators.
   virtual bool pickMorsel() { return false; }

   /// Build a unique identifier for this suboperator (unique given the parameter set).
   /// This is needed to effectively use the fragment cache during vectorized interpretation.
   virtual std::string id() const = 0;
   /// Get a variable identifier which is unique to this suboperator.
   std::stringstream getVarIdentifier() const;
   /// Build the name for a given iu.
   std::string buildIUName(IUScoped iu) const;

   /// How many ius does this suboperator depend on?
   size_t getNumSourceIUs() const { return source_ius.size(); }
   const std::set<const IU*>& getSourceIUs() const { return source_ius; }
   const std::set<const IU*>& getIUs() const { return provided_ius; }

   protected:

   /// The operator which decayed into this Suboperator.
   const RelAlgOp* source;

   /// IUs produced by this sub-operator.
   std::set<const IU*> provided_ius;
   /// Source IUs on which this sub-operator depends.
   std::set<const IU*> source_ius;

};

using SuboperatorArc = std::shared_ptr<Suboperator>;

}

#endif //INKFUSE_SUBOPERATOR_H
