#ifndef INKFUSE_PIPELINE_H
#define INKFUSE_PIPELINE_H

#include "algebra/suboperators/Suboperator.h"
#include <memory>
#include <vector>

namespace inkfuse {

/// Execution pipeline containing all the different operators within one pipeline.
/// This class is at the heart of inkfuse as it allows either vectorized interpretation
/// through pre-compiled fragments or just-in-time-compiled loop fusion.
/// Internally, the pipeline is a DAG of suboperators. For the time being, we simply represent it as a
/// layer of suboperators, where every suboperator is conntected to all suboperators in the next layer.
struct Pipeline {
public:

    /// Attach a new layer rooted in the given suboperator.
    void attachLayer(SuboperatorPtr op);

private:
    /// The suboperators which have to be executed. Every element in the outer vector is a
    /// set of suboperators which need to be fully executed before the next layer can commence.
    std::vector<SuboperatorPtr> layers;

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
