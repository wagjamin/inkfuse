#ifndef INKFUSE_PIPELINERUNNER_H
#define INKFUSE_PIPELINERUNNER_H

#include "algebra/Pipeline.h"
#include "exec/ExecutionContext.h"
#include <string>
#include <vector>
#include <memory>

namespace inkfuse {

/// Base class for a pipeline runner. A pipeline runner
/// takes a backing pipeline and executed it either through vectorization,
/// or interpretation.
/// One thing is important here - in most cases, the pipeline which gets put into a
/// runner is not the full pipeline the relalg operators decayed into. Rather, we can repipe
/// arbitrary intervals of the original pipeline into a new pipeline and pass them into a new runner.
struct PipelineRunner {

   /// Constructor, also sets up the necessary execution state.
   PipelineRunner(PipelinePtr pipe_, ExecutionContext& context_);

   virtual ~PipelineRunner() = default;

   /// Run a single morsel of the backing pipeline.
   /// @return whether there are more morsels to execute.
   bool runMorsel();

   /// Prepare the runner, this can include steps like code generation.
   virtual void prepare() { prepared = true; };

   protected:
   /// Set up the state for the given pipeline.
   void setUpState();

   /// The compiled function. Either a fragment received from the backing cache,
   /// or a new function.
   std::function<uint8_t(void**, void**, void*)> fct;
   /// Operator states for this specific pipeline.
   std::vector<void*> states;
   /// The backing pipeline.
   PipelinePtr pipe;
   /// The backing execution context.
   ExecutionContext& context;
   /// Was this runner prepared already?
   bool prepared = false;
};

using PipelineRunnerPtr = std::unique_ptr<PipelineRunner>;


}

#endif //INKFUSE_PIPELINERUNNER_H
