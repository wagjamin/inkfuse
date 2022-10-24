#ifndef INKFUSE_PIPELINE_H
#define INKFUSE_PIPELINE_H

#include "algebra/suboperators/Suboperator.h"
#include "exec/FuseChunk.h"
#include "runtime/HashTables.h"
#include <map>
#include <memory>
#include <unordered_set>
#include <vector>
#include <deque>

namespace inkfuse {

struct PrettyPrinter;

/// Pipelines are DAG structured through IU dependencies. The PipelineGraph explicitly
/// models the edges induced by IU dependencies between sub-operators.
/// Strong edges are modeled directly through the sub-operator properties on "incoming edges".
/// The strong link is always at index 0 of the sub-operator vector.
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

   /// Rebuild the pipeline for a subset of the sub-operators in the given range.
   /// Inserts fake sources and fake sinks. Materializes all IUs that are produced by nodes that don't have a strong output link.
   std::unique_ptr<Pipeline> repipeAll(size_t start, size_t end) const;
   /// Rebuild the pipeline for a subset of the sub-operators in the given range.
   /// Inserts fake sources and fake sinks. Materializes all IUs that are used by followup operators.
   std::unique_ptr<Pipeline> repipeRequired(size_t start, size_t end) const;
   /// Rebuild the pipeline for a subset of the sub-operators in the given range.
   /// Inserts fake sources and fake sinks. Materializes the IUs in 'materialize'.
   std::unique_ptr<Pipeline> repipe(size_t start, size_t end, const std::unordered_set<const IU*>& materialize) const;

   /// Get the downstream consumers of IUs for a given sub-operator.
   const std::vector<Suboperator*>& getConsumers(Suboperator& subop) const;
   /// Get the upstream producers of IUs for a given sub-operator.
   const std::vector<Suboperator*>& getProducers(Suboperator& subop) const;

   /// Get the suboperator producing a given IU.
   Suboperator& getProvider(const IU& iu) const;
   Suboperator* tryGetProvider(const IU& iu) const;

   /// Add a new sub-operator to the pipeline.
   Suboperator& attachSuboperator(SuboperatorArc subop);

   /// Get the sub-operators in this pipeline.
   const std::vector<SuboperatorArc>& getSubops() const;

   void setPrettyPrinter(PrettyPrinter& printer);
   PrettyPrinter* getPrettyPrinter();

   private:
   friend class CompilationContext;
   friend class ExecutionContext;
   friend class PipelineRunner;

   /// The sub-operators within this pipeline. These are arranged in a topological order of the backing
   /// DAG structure.
   std::vector<SuboperatorArc> suboperators;

   /// Explicit graph structure induced by the sub-operators.
   PipelineGraph graph;
   /// The IU providers.
   std::unordered_map<const IU*, Suboperator*> iu_providers;

   /// An optional PrettyPrinter that can be used to render the pipeline results.
   PrettyPrinter* printer;
};

using PipelinePtr = std::unique_ptr<Pipeline>;

/// The PipelineDAG represents a query which is ready for execution.
/// It also contains the runtime state of a query. This is not a very clean abstraction at the moment.
/// It would be much cleaner to have this in the ExecutionContext.
struct PipelineDAG {
   public:
   /// Get the current pipeline.
   Pipeline& getCurrentPipeline() const;

   Pipeline& buildNewPipeline();

   const std::vector<PipelinePtr>& getPipelines() const;

   /// Mark a pipeline as done. Allows for the release of runtime state.
   void markPipelineDone(size_t idx);

   /// Attach a hash table to the runtime state of the PipelineDAG.
   HashTableSimpleKey& attachHashTableSimpleKey(size_t discard_after, size_t key_size, size_t payload_size);

   private:
   /// Internally the PipelineDAG is represented as a vector of pipelines within a topological order.
   std::vector<PipelinePtr> pipelines;
   /// Hash tables, ordered by pipeline after which they can be discarded.
   std::deque<std::pair<size_t, std::unique_ptr<HashTableSimpleKey>>> hash_tables;
};

using PipelineDAGPtr = std::unique_ptr<PipelineDAG>;

}

#endif //INKFUSE_PIPELINE_H
