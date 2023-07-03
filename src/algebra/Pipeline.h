#ifndef INKFUSE_PIPELINE_H
#define INKFUSE_PIPELINE_H

#include "algebra/suboperators/Suboperator.h"
#include "exec/DeferredState.h"
#include "exec/FuseChunk.h"
#include "runtime/TupleMaterializer.h"
#include <deque>
#include <map>
#include <memory>
#include <unordered_set>
#include <vector>

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
struct PipelineDAG {
   public:
   /// Get the current pipeline.
   Pipeline& getCurrentPipeline() const;

   Pipeline& buildNewPipeline();

   struct RuntimeTask {
      /// After what pipeline should this task be run?
      size_t after_pipe;
      /// Single threaded prepare (e.g. allocate hash table).
      std::function<void(size_t)> prepare_function;
      /// Multi-threaded worker (e.g. populate hash table).
      std::function<void(size_t)> worker_function;
   };
   /// Add a runtime task that should be run on all worker threads after a given pipeline id.
   void addRuntimeTask(RuntimeTask task);
   const std::vector<RuntimeTask>& getRuntimeTasks() const;

   const std::vector<PipelinePtr>& getPipelines() const;

   /// Mark a pipeline as done. Allows for the release of runtime state.
   void markPipelineDone(size_t idx);

   /// Attach tuple materializers to the runtime state of the PipelineDAG.
   TupleMaterializerState& attachTupleMaterializers(size_t discard_after, size_t tuple_size);
   /// Attach an atomic hash table that supports parallel inserts & lookups in two phases.
   template <class Comparator>
   AtomicHashTableState<Comparator>& attachAtomicHashTable(size_t discard_after, TupleMaterializerState& materialize_) {
      auto& inserted = runtime_state.emplace_back(discard_after, std::make_unique<AtomicHashTableState<Comparator>>(materialize_));
      return static_cast<AtomicHashTableState<Comparator>&>(*inserted.second);
   };

   /// Attach a simple hash table to the runtime state of the PipelineDAG.
   HashTableSimpleKeyState& attachHashTableSimpleKey(size_t discard_after, size_t key_size, size_t payload_size);
   /// Attach a complex hash table to the runtime state of the PipelineDAG.
   HashTableComplexKeyState& attachHashTableComplexKey(size_t discard_after, uint16_t slots, size_t payload_size);
   /// Attach a direct lookup hash table to the runtime state of the PipelineDAG.
   HashTableDirectLookupState& attachHashTableDirectLookup(size_t discard_after, size_t payload_size);

   private:
   /// Internally the PipelineDAG is represented as a vector of pipelines within a topological order.
   std::vector<PipelinePtr> pipelines;
   /// Runtime tasks to schedule once certain pipelines are fully executed.
   std::vector<RuntimeTask> runtime_tasks;

   /// Deferred state initializers for runtime state.
   std::deque<std::pair<size_t, std::unique_ptr<DefferredStateInitializer>>> runtime_state;
};

using PipelineDAGPtr = std::unique_ptr<PipelineDAG>;

}

#endif //INKFUSE_PIPELINE_H
