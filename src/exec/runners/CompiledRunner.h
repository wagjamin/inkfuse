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

   void generateC();
   bool generateMachineCode(InterruptableJob& interrupt);

   private:
   // The compilation backend.
   BackendC backend;
   /// The backing program.
   std::unique_ptr<IR::BackendProgram> program;
   /// Name of the pipeline/program to be generated.
   std::string name;
};

}

#endif //INKFUSE_COMPILEDRUNNER_H
