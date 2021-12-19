#ifndef INKFUSE_PIPELINE_H
#define INKFUSE_PIPELINE_H

#include <memory>
#include <vector>
#include "algebra/Operator.h"

namespace inkfuse {

    /// Execution pipeline containing all the different operators within one pipeline.
    /// This class is at the heart of inkfuse as it allows either vectorized interpretation
    /// through pre-compiled fragments or just-in-time-compiled loop fusion.
    struct Pipeline {
    private:
        /// Pipeline root.
        OperatorPtr root;

    public:

    };

    using PipelinePtr = std::unique_ptr<Pipeline>;

    /// The pipeline dag represents a query which is ready for execution.
    struct PipelineDAG {
    private:
        /// Internally the PipelineDAG is represented as a vector of pipelines within a topological order.
        std::vector<PipelinePtr> pipelines;

    public:

    };
}

#endif //INKFUSE_PIPELINE_H
