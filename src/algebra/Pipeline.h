#ifndef INKFUSE_PIPELINE_H
#define INKFUSE_PIPELINE_H

#include "algebra/suboperators/Suboperator.h"
#include "exec/ExecutionContext.h"
#include <memory>
#include <vector>

namespace inkfuse {

/// Execution pipeline containing all the different operators within one pipeline.
/// This class is at the heart of inkfuse as it allows either vectorized interpretation
/// through pre-compiled fragments or just-in-time-compiled loop fusion.
/// Internally, the pipeline is a DAG of suboperators.
struct Pipeline {
public:

    /// Add a new suboperator to the pipeline.
    void attachSuboperator(SuboperatorPtr subop);

private:
    /// The sub-operators within this pipeline. These are arranged in a topological order of the backing
    /// DAG structure. This is important down the line .
    std::vector<SuboperatorPtr> suboperators;

    /// The offsets of the sink sub-operators of this pipeline. These are the ones where interpretation needs to begin.
    std::set<size_t> sinks;

    /// The offsets of operators which re-scope the pipeline.
    std::vector<size_t> rescope_offsets;

    /// The execution context of this pipeline.
    ExecutionContext context;
};

    using PipelinePtr = std::unique_ptr<Pipeline>;

/// The PipelineDAG represents a query which is ready for execution.
struct PipelineDAG {
   public:

   private:
   /// Internally the PipelineDAG is represented as a vector of pipelines within a topological order.
   std::vector<PipelinePtr> pipelines;
};

using PipelineDAGPtr = std::unique_ptr<PipelineDAG>;

}

#endif //INKFUSE_PIPELINE_H
