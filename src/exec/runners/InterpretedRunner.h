#ifndef INKFUSE_PIPELINEINTERPRETER_H
#define INKFUSE_PIPELINEINTERPRETER_H

#include "PipelineRunner.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace inkfuse {

/// The pipeline intepreter receives a single pipeline
struct InterpretedRunner final : public PipelineRunner {

   /// Create a pipeline interpreter which will interpret the sub-operator at the given index.
   InterpretedRunner(const Pipeline& backing_pipeline, size_t idx, ExecutionContext& original_context);

   private:
   /// Get the properly repiped pipeline for the actual execution.
   static PipelinePtr getRepiped(const Pipeline& backing_pipeline, size_t idx);
};

}

#endif //INKFUSE_PIPELINEINTERPRETER_H
