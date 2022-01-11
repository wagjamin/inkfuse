#ifndef INKFUSE_PIPELINE_H
#define INKFUSE_PIPELINE_H

#include "algebra/Suboperator.h"
#include <memory>
#include <vector>

namespace inkfuse {

/// Execution pipeline containing all the different operators within one pipeline.
/// This class is at the heart of inkfuse as it allows either vectorized interpretation
/// through pre-compiled fragments or just-in-time-compiled loop fusion.
/// Internally, the pipeline is a DAG of suboperators. For the time being, we simply represent it as a
/// linearized DAG (i.e. a topological sort of the DAG).
struct Pipeline {
   private:
   public:
};

using PipelinePtr = std::unique_ptr<Pipeline>;

/// The PipelineDag represents a query which is ready for execution.
struct PipelineDag {
   public:
   private:
   /// Internally the PipelineDAG is represented as a vector of pipelines within a topological order.
   std::vector<PipelinePtr> pipelines;
};

using PipelineDagPtr = std::unique_ptr<PipelineDag>;
}

#endif //INKFUSE_PIPELINE_H
