#ifndef INKFUSE_PIPELINEEXECUTOR_H
#define INKFUSE_PIPELINEEXECUTOR_H

#include "exec/ExecutionContext.h"
#include <string>
#include <functional>
#include <vector>
#include <map>

namespace inkfuse {

struct Pipeline;

/// The pipeline executor receives a single pipeline and executes it.
struct PipelineExecutor {

   PipelineExecutor(const Pipeline& pipe_, std::string name = "");

   /// Run the full pipeline. The pipeline executor can choose whether
   /// to interpret, fuse, or do adaptive execution.
   void run();
   /// Run the pipeline in fused mode.
   void runFused(std::optional<size_t> morsels = {});
   /// Run the pipeline in interpreted mode.
   void runInterpreted();
   /// Get the execution context of this executor.
   /// Mainly needed for testing.
   const ExecutionContext& getExecutionContext();

   /// Set up the state, i.e. the execution context.
   void setUpState();
   /// Tear down the state again.
   void tearDownState();

   private:
   /// Run a single morsel in fused mode.
   bool runFusedMorsel();
   /// Set up the state array for a subset of operators if it was not.
   /// The slice is in the interval [start, end[. Returns the set up state.
   void** setUpState(size_t start, size_t end);

   /// The generated function.
   std::function<uint8_t(void**, void**, void*)> fct;
   /// The pipeline which has to be executed.
   const Pipeline& pipe;
   /// The execution context.
   ExecutionContext context;
   /// Name of the pipeline/program to be generated.
   std::string name;
   /// The prepared states for different operator intervals.
   std::map<std::pair<size_t, size_t>, std::vector<void*>> states;
};

}

#endif //INKFUSE_PIPELINEEXECUTOR_H
