#ifndef INKFUSE_COMPILEDRUNNER_H
#define INKFUSE_COMPILEDRUNNER_H

#include "PipelineRunner.h"
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

   // TODO Fix interface.
   void prepare() override {
      throw std::runtime_error("Not implemented.");
   };

   // Prepare the compiled runner, returns whether compilation finished successfully.
   bool prepare(InterruptableJob& interrupt);

   private:
   /// Name of the pipeline/program to be generated.
   std::string name;
};

}

#endif //INKFUSE_COMPILEDRUNNER_H
