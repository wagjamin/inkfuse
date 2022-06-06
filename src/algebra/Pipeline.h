#ifndef INKFUSE_PIPELINE_H
#define INKFUSE_PIPELINE_H

#include "algebra/IUScope.h"
#include "algebra/suboperators/Suboperator.h"
#include "exec/FuseChunk.h"
#include <map>
#include <memory>
#include <set>
#include <vector>

namespace inkfuse {

/// Pipelines are DAG structured through IU dependencies. The PipelineGraph explicitly
/// models the edges induced by IU dependencies between sub-operators.
/// Strong edges are modeled directly through the sub-operator properties on "incoming edges".
struct PipelineGraph {
   /// Incoming edges into a given sub-operator.
   std::unordered_map<const Suboperator*, std::vector<Suboperator*>> incoming_edges;
   /// Outgoing edges out of a given sub-operator.
   std::unordered_map<const Suboperator*, std::vector<Suboperator*>> outgoing_edges;
};

/// Execution pipeline containing all the different operators within one pipeline.
/// This class is at the heart of inkfuse as it allows either vectorized interpretation
/// through pre-compiled fragments or just-in-time-compiled loop fusion.
/// Internally, the pipeline is a DAG of suboperators.
struct Pipeline {
   public:
   /// Constructor which creates a root scope.
   Pipeline();

   /// Rebuild the pipeline for a subset of the sub-operators in the given range.
   /// Inserts face sources and fake sinks.
   std::unique_ptr<Pipeline> repipe(size_t start, size_t end, bool materialize_all = false) const;

   /// Get the current scope.
   const IUScope& getScope(size_t id) const;

   /// Get the downstream consumers of IUs for a given sub-operator.
   const std::vector<Suboperator*>& getConsumers(Suboperator& subop) const;
   /// Get the upstream producers of IUs for a .
   const std::vector<Suboperator*>& getProducers(Suboperator& subop) const;

   /// Get the IU provider in a certain scope.
   Suboperator& getProvider(size_t scope_idx, const IU& iu) const;
   Suboperator* tryGetProvider(size_t scope_idx, const IU& iu) const;

   /// Resolve the scope of a given operator.
   /// Incoming indicates whether the scope should be resolved for incoming
   /// our outgoing edges.
   size_t resolveOperatorScope(const Suboperator& op, bool incoming = true) const;

   /// Add a new sub-operator to the pipeline.
   Suboperator& attachSuboperator(SuboperatorArc subop);

   /// Get the sub-operators in this pipeline.
   const std::vector<SuboperatorArc>& getSubops() const;

   private:
   friend class CompilationContext;
   friend class ExecutionContext;
   friend class PipelineRunner;

   /// The sub-operators within this pipeline. These are arranged in a topological order of the backing
   /// DAG structure.
   std::vector<SuboperatorArc> suboperators;

   /// Explicit graph structure induced by the sub-operators.
   PipelineGraph graph;

   /// The offsets of operators which re-scope the pipeline.
   std::vector<size_t> rescope_offsets;

   /// The scopes of this pipeline.
   std::vector<IUScopeArc> scopes;
};

using PipelinePtr = std::unique_ptr<Pipeline>;

/// The PipelineDAG represents a query which is ready for execution.
struct PipelineDAG {
   public:
   /// Get the current pipeline.
   Pipeline& getCurrentPipeline() const;

   Pipeline& buildNewPipeline();

   const std::vector<PipelinePtr>& getPipelines() const;

   private:
   /// Internally the PipelineDAG is represented as a vector of pipelines within a topological order.
   std::vector<PipelinePtr> pipelines;
};

using PipelineDAGPtr = std::unique_ptr<PipelineDAG>;

}

#endif //INKFUSE_PIPELINE_H
