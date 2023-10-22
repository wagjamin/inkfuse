#ifndef INKFUSE_PIPELINEEXECUTOR_H
#define INKFUSE_PIPELINEEXECUTOR_H

#include "algebra/Pipeline.h"
#include "algebra/RelAlgOp.h"
#include "common/Barrier.h"
#include "exec/InterruptableJob.h"
#include "exec/runners/CompiledRunner.h"
#include "exec/runners/PipelineRunner.h"
#include <future>
#include <map>
#include <utility>

namespace inkfuse {

namespace QueryExecutor {
struct StepwiseExecutor;
};

struct InterruptableJob;

/// The pipeline executor takes a full pipeline and runs it.
/// It uses the PipelineRunners in the background to execute a fraction of the overall pipeline.
struct PipelineExecutor {
   /// Control block keeping a relational algebra tree alive until all pipelines and
   /// their spawned background tasks are done executing.
   struct QueryControlBlock {
      explicit QueryControlBlock(RelAlgOpPtr root_) : root(std::move(root_)) {
         root->decay(dag);
      }

      /// The root of the algebra tree.
      RelAlgOpPtr root;
      /// The PipelineDAG that was produced by the algebra tree.
      PipelineDAG dag;
   };
   using QueryControlBlockArc = std::shared_ptr<QueryControlBlock>;

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
   PipelineExecutor(Pipeline& pipe_, size_t num_threads_, ExecutionMode mode_ = ExecutionMode::Hybrid, std::string full_name_ = "", QueryControlBlockArc control_block_ = nullptr);

   ~PipelineExecutor() noexcept;

   const ExecutionContext& getExecutionContext() const;

   /// Prepare the pipeline. Triggers asynchronous compilation for
   /// compiled/hybrid mode.
   /// Does not have to be called before `runPipeline`, but allows kicking
   /// off asynchronous preparation work.
   void preparePipeline(ExecutionMode prep_mode);

   /// Statistics about the execution of a finished pipeline.
   struct PipelineStats {
      /// How many microseconds was execution stalled on waiting for code generation?
      size_t codegen_microseconds = 0;
      /// How much time was spent in runtime tasks (single threaded part)?
      size_t runtime_microseconds_st = 0;
      /// How much time was spent in runtime tasks (multi threaded part)?
      size_t runtime_microseconds_mt = 0;
   };
   /// Run the full pipeline to completion.
   PipelineStats runPipeline();

   /// Run only a single morsel.
   /// @return true if there are more morsels.
   Suboperator::PickMorselResult runMorsel(size_t thread_id);

   private:
   /// Run a full morsel through the compiled path.
   Suboperator::PickMorselResult runFusedMorsel(size_t thread_id);
   /// Run a full morsel through the interpreted path.
   Suboperator::PickMorselResult runInterpretedMorsel(size_t thread_id);

   /// After preparation in `runPipeline`, schedules the worker threads performing
   /// query processing. Waits for all worker threads to be done.
   void runSwimlanes();
   /// Execution loop for a thread. Called after preparation in `runPipeline`.
   /// @param compile_prep_barrier If we are in hybrid mode, a OnceBarrier is used
   ///            to synchronize all threads around preparation of the compiled runner.
   void threadSwimlane(size_t thread_id, OnceBarrier& compile_prep_barrier);

   /// Set up interpreted state in a synchronous way.
   void setUpInterpreted();
   /// Set up fused state in an asynchronous way.
   /// Returns a handle to a thread performing asynchronous compilation.
   std::thread setUpFusedAsync();
   /// Clean up the fuse chunks for a new morsel.
   void cleanUp(size_t thread_id);

   /// Asynchronous state used for background compilation that may outlive this PipelineExecutor.
   struct AsyncCompileState {
      AsyncCompileState(QueryControlBlockArc control_block_, Pipeline& pipe, size_t num_threads_)
         : context(pipe, num_threads_), control_block(std::move(control_block_)){};

      /// Lock protecting shared state with the background thread doing async compilation.
      std::mutex compiled_lock;
      /// Compiled fragments identified by [start, end[ index pairs.
      std::unique_ptr<CompiledRunner> compiled;
      /// Control block - frees relational algebra tree once all pipelines including their
      /// async compilation is done.
      QueryControlBlockArc control_block;
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
   /// Suboperator offsets of the individual interpreters.
   std::vector<size_t> interpreter_offsets;
   /// Backing execution mode.
   ExecutionMode mode;
   /// Potential full name of the generated program.
   std::string full_name;
   /// Was the pipeline set-up started for the interpreted mode?
   bool interpreter_setup_started = false;
   /// Was the pipeline set-up started for the compiled mode?
   bool compiler_setup_started = false;

   /// The background thread performing compilation.
   std::thread compilation_job;

   /// Query executor is responsible for running runtime tasks. It needs access to the
   /// ExecutionContext.
   friend struct QueryExecutor::StepwiseExecutor;
   friend struct RechunkBarrier;
};

}

#endif //INKFUSE_PIPELINEEXECUTOR_H
