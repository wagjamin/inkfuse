#ifndef INKFUSE_QUERYEXECUTOR_H
#define INKFUSE_QUERYEXECUTOR_H

#include "exec/PipelineExecutor.h"

namespace inkfuse {

/// Central query executor responsible for executing a query through incremental fusion.
namespace QueryExecutor {

/// Run a complete query to completion. Returns aggregated (summed) pipeline statistics.
PipelineExecutor::PipelineStats runQuery(PipelineExecutor::QueryControlBlockArc control_block_, PipelineExecutor::ExecutionMode mode, const std::string& qname = "query");

};

}

#endif //INKFUSE_QUERYEXECUTOR_H
