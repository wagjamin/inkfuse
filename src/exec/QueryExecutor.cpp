#include "exec/QueryExecutor.h"
#include "algebra/Pipeline.h"

namespace inkfuse
{
    
void QueryExecutor::runQuery(const PipelineDAG& dag, PipelineExecutor::ExecutionMode mode, const std::string& qname) {
    const auto& pipes = dag.getPipelines();
    for (size_t idx = 0; idx < pipes.size(); ++idx) {
        const auto& pipe = pipes[idx];
        PipelineExecutor exec(*pipe, mode, qname + "_pipe_" + std::to_string(idx));
        exec.runPipeline();
    }
}

} // namespace inkfuse
