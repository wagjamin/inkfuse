#ifndef INKFUSE_COMPILEDRUNNER_H
#define INKFUSE_COMPILEDRUNNER_H

#include "PipelineRunner.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace inkfuse {

struct Pipeline;

/// The compiled runner receives a pipeline and executes it
/// through operator fusion.
struct CompiledRunner final : public PipelineRunner  {
   CompiledRunner(PipelinePtr pipe_, ExecutionContext& ctx_, std::string name = "");

   void prepare() override;

   private:
   /// Name of the pipeline/program to be generated.
   std::string name;
};

}

#endif //INKFUSE_COMPILEDRUNNER_H
