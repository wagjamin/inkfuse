#ifndef INKFUSE_PIPELINE_H
#define INKFUSE_PIPELINE_H

#include "exec/FuseChunk.h"
#include "algebra/suboperators/Suboperator.h"
#include <memory>
#include <vector>
#include <set>

namespace inkfuse {

/// In inkfuse, a scope represents the maximum number of suboperators during whose execution
/// a selection vector remains valid. When leaving a scope, a new FuseChunk and a new selection vector
/// are introduced. As such, all suboperators which do not necessarily represent a 1:1 mapping between
/// an input and and output tuple generate a new scope.
/// Note that this corresponds very closely to actual scopes in the generated code.
struct Scope {
   /// Create a new scope which re-uses an old chunk.
   Scope(size_t id_, std::shared_ptr<FuseChunk> chunk_) : selection(std::make_unique<IU>(IR::Bool::build(), "sel_scope" + std::to_string(id))),
                                                          id(id_),
                                                          chunk(std::move(chunk_)) {
      // We need a column containing the current selection vector.
      chunk->attachColumn(*selection);
   }

   /// Create a completely new scope with a clean chunk.
   Scope(size_t id_) : Scope(id_, std::make_shared<FuseChunk>()){};

   /// Register an IU producer within the given scope.
   void registerProducer(const IU& iu, Suboperator& op);

   /// Get the producing sub-operator of an IU within the given scope.
   Suboperator& getProducer(const IU& iu) const;

   /// Get the raw data column for the given IU.
   Column& getColumn(const IU& iu) const;

   /// Get the raw data column for the selection vector.
   Column& getSel() const;

   /// Scope id.
   size_t id;
   /// Boolean IU for the selection vector. We need a pointer to retain IU stability.
   std::unique_ptr<IU> selection;
   /// Backing chunk of data within this scope. For efficiency reasons, a new scope somtimes can refer
   /// to the same backing FuseChunk as the previous ones. This is done to ensure that a filter sub-operator
   /// does not have to copy all data.
   std::shared_ptr<FuseChunk> chunk;
   /// A map from IUs to the producing operators.
   std::unordered_map<const IU*, Suboperator*> iu_producers;
};

using ScopePtr = std::unique_ptr<Scope>;

/// Execution pipeline containing all the different operators within one pipeline.
/// This class is at the heart of inkfuse as it allows either vectorized interpretation
/// through pre-compiled fragments or just-in-time-compiled loop fusion.
/// Internally, the pipeline is a DAG of suboperators.
struct Pipeline {
   public:
   /// Constructor which creates a root scope.
   Pipeline();

   /// Get the raw data given a scoped IU.
   Column& getScopedIU(IUScoped iu);

   /// Get the selection vector for a given scope.
   Column& getSelectionCol(size_t scope_id);

   /// Get the IU provider in a certain scope.
   Suboperator& getProvider(IUScoped iu);

   /// Resolve the scope of a given operator.
   size_t resolveOperatorScope(const Suboperator& op) const;

   /// Add a new sub-operator to the pipeline.
   void attachSuboperator(std::unique_ptr<Suboperator> subop);

   private:
   friend class CompilationContext;

   /// The sub-operators within this pipeline. These are arranged in a topological order of the backing
   /// DAG structure.
   std::vector<std::unique_ptr<Suboperator>> suboperators;

   /// The offsets of the sink sub-operators of this pipeline. These are the ones where interpretation needs to begin.
   std::set<size_t> sinks;

   /// The offsets of operators which re-scope the pipeline.
   std::vector<size_t> rescope_offsets;

   /// The scopes of this pipeline.
   std::vector<ScopePtr> scopes;
};

using PipelinePtr = std::unique_ptr<Pipeline>;

/// The PipelineDAG represents a query which is ready for execution.
struct PipelineDAG {
   public:
   /// Get the current pipeline.
   Pipeline& getCurrentPipeline() const;

   Pipeline& buildNewPipeline() const;

   private:
   /// Internally the PipelineDAG is represented as a vector of pipelines within a topological order.
   std::vector<PipelinePtr> pipelines;
};

using PipelineDAGPtr = std::unique_ptr<PipelineDAG>;

}

#endif //INKFUSE_PIPELINE_H
