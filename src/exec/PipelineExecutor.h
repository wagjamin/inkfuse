#ifndef INKFUSE_PIPELINEEXECUTOR_H
#define INKFUSE_PIPELINEEXECUTOR_H

#include "algebra/Pipeline.h"
#include "exec/InterruptableJob.h"
#include "exec/runners/CompiledRunner.h"
#include "exec/runners/PipelineRunner.h"
#include <map>
#include <utility>
#include <future>

namespace inkfuse {

struct InterruptableJob;

/// The pipeline executor takes a full pipeline and runs it.
/// It uses the PipelineRunners in the background to execute a fraction of the overall pipeline.
struct PipelineExecutor {

   /// Which execution mode should be chosen for this pipeline?
   enum class ExecutionMode : uint8_t {
      /// In fused mode, the complete pipeline gets compiled and then executed.
      Fused,
      /// In interpreted mode, we only use the pre-compiled fragments to run the query.
      Interpreted,
      /// In hybrid mode, we switch between fused and interpreted execution based on runtime statistics.
      Hybrid
   };

   /// Create a new pipeline executor.
   /// @param pipe_ the backing pipe to be executed
   /// @param mode_ the execution mode to execute the pipeline in
   /// @param full_name_ the name of the full compiled binary
   PipelineExecutor(Pipeline& pipe_, ExecutionMode mode_ = ExecutionMode::Hybrid, std::string full_name_ = "");

   ~PipelineExecutor() noexcept;

   const ExecutionContext& getExecutionContext() const;

   /// Prepare the pipeline. Triggers asynchronous compilation for
   /// compiled/hybrid mode.
   /// Does not have to be called before `runPipeline`, but allows kicking
   /// off asynchronous preparation work.
   void preparePipeline();

   /// Run the full pipeline to completion.
   void runPipeline();

   /// Run only a single morsel.
   /// @return true if there are more morsels.
   bool runMorsel();

   private:
   /// Run a full morsel through the compiled path.
   bool runFusedMorsel();
   /// Run a full morsel through the interpreted path.
   bool runInterpretedMorsel();

   /// Set up interpreted state in a synchronous way.
   void setUpInterpreted();
   /// Set up fused state in an asynchronous way.
   /// Returns a handle to a thread performing asynchronous compilation.
   std::thread setUpFusedAsync();
   /// Clean up the fuse chunks for a new morsel.
   void cleanUp();

   /// Asynchronous state used for background compilation that may outlive this PipelineExecutor.
   struct AsyncCompileState {
      AsyncCompileState(Pipeline& pipe) : context(pipe) {};

      /// Lock protecting shared state with the background thread doing async compilation.
      std::mutex compiled_lock;
      /// Compiled fragments identified by [start, end[ index pairs.
      std::unique_ptr<CompiledRunner> compiled;
      /// Execution context. Unified across all runners as fuse chunks have to be shared.
      ExecutionContext context;
      /// Compilation interrupt. Allows aborting compilation if interpretation
      /// is done before the compiled code is ready.
      InterruptableJob interrupt;
      /// Was fused mode set up successfully? Atomic since this is done asynchronously.
      bool fused_set_up = false;
   };

   /// Shared compilation state with the background thread doing code generation.
   std::shared_ptr<AsyncCompileState> compile_state;
   /// Backing pipeline.
   Pipeline& pipe;
   /// Interpreters for the different sub-operators.
   std::vector<PipelineRunnerPtr> interpreters;
   /// Backing execution mode.
   ExecutionMode mode;
   /// Potential full name of the generated program.
   std::string full_name;
   /// Was interpreted mode set up successfully?
   bool interpreted_set_up = false;

   /// The background thread performing compilation.
   std::thread compilation_job;
};

}

#endif //INKFUSE_PIPELINEEXECUTOR_H
