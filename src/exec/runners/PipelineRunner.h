#ifndef INKFUSE_PIPELINERUNNER_H
#define INKFUSE_PIPELINERUNNER_H

#include "algebra/Pipeline.h"
#include "exec/ExecutionContext.h"
#include <memory>
#include <string>
#include <vector>

namespace inkfuse {

/// Base class for a pipeline runner. A pipeline runner
/// takes a backing pipeline and executed it either through vectorization,
/// or interpretation.
/// One thing is important here - in most cases, the pipeline which gets put into a
/// runner is not the full pipeline the relalg operators decayed into. Rather, we can repipe
/// arbitrary intervals of the original pipeline into a new pipeline and pass them into a new runner.
struct PipelineRunner {
   /// Constructor, also sets up the necessary execution state.
   /// @param pipe_ the pipeline to run
   /// @param old_context the execution context of the original pipeline that created this runner
   PipelineRunner(PipelinePtr pipe_, ExecutionContext& old_context);

   virtual ~PipelineRunner() = default;

   /// Pick a morsel of the backing pipeline.
   virtual Suboperator::PickMorselResult pickMorsel(size_t thread_id);

   /// Run a previously picked morsel of the backing pipeline.
   /// @param force_pick should we always pick, even if we are not a fuse chunk source?
   /// @return result of picking a morsel.
   virtual void runMorsel(size_t thread_id);

   /// Clean up the intermediate morsel state from a previous failure.
   /// Purges the morsel size of the sinks to make sure we get a fresh
   /// column to write into.
   void prepareForRerun(size_t thread_id);

   /// Set up the state for the given pipeline.
   /// @param compiled_hybrid are we setting up state as the compiled backend in hybird mode?
   void setUpState();

   protected:
   /// The compiled function. Either a fragment received from the backing cache,
   /// or a new function.
   std::function<uint8_t(void**)> fct;
   /// Every thread has one state object for every suboperator in the pipeline.
   using ThreadLocalState = std::vector<void*>;
   /// Operator states for this specific pipeline.
   std::vector<ThreadLocalState> states;
   /// The backing pipeline.
   PipelinePtr pipe;
   /// The recontextualized execution context.
   ExecutionContext context;
   /// Is this pipeline driven by a fuse chunk source?
   bool fuseChunkSource;
   /// Was this runner prepared already?
   bool prepared = false;
   /// Was the state of this runner set up already?
   bool set_up = false;
};

using PipelineRunnerPtr = std::unique_ptr<PipelineRunner>;
}

#endif //INKFUSE_PIPELINERUNNER_H
