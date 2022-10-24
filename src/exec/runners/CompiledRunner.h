#ifndef INKFUSE_COMPILEDRUNNER_H
#define INKFUSE_COMPILEDRUNNER_H

#include "PipelineRunner.h"
#include "codegen/backend_c/BackendC.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace inkfuse {

struct Pipeline;
struct InterruptableJob;

/// The compiled runner receives a pipeline and executes it
/// through operator fusion.
struct CompiledRunner final : public PipelineRunner  {
   CompiledRunner(PipelinePtr pipe_, ExecutionContext& ctx_, std::string name = "");

   // Prepare the compiled runner, returns whether compilation finished successfully.
   bool prepare(InterruptableJob& interrupt);

   private:
   /// Name of the pipeline/program to be generated.
   std::string name;
   /// The backing program.
   std::unique_ptr<IR::BackendProgram> program;
};

}

#endif //INKFUSE_COMPILEDRUNNER_H
