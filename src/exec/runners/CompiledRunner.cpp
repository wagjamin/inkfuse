#include "CompiledRunner.h"
#include "algebra/CompilationContext.h"
#include "codegen/backend_c/BackendC.h"
#include "exec/InterruptableJob.h"
#include <atomic>

namespace inkfuse {

namespace {

std::atomic<size_t> runner_id = 0;

}

CompiledRunner::CompiledRunner(PipelinePtr pipe_, ExecutionContext& context_, std::string name_)
   : PipelineRunner(std::move(pipe_), context_), name(std::move(name_))
{
   if (name.empty()) {
      name = "pipeline_" + std::to_string(runner_id++);
   }
}

void CompiledRunner::generateC()
{
   // Create IR program for the pipeline.
   CompilationContext comp(name, *pipe);
   comp.compile();
   // Generate C code.
   program = backend.generate(comp.getProgram());
}

bool CompiledRunner::generateMachineCode(InterruptableJob& interrupt)
{
   program->compileToMachinecode(interrupt);
   if (interrupt.getResult() == InterruptableJob::Change::JobDone) {
      // If we finished successfully without interrupt we can fetch the generated function.
      fct = reinterpret_cast<uint8_t(*)(void**)>(program->getFunction("execute"));
      assert(fct);
      prepared = true;
   }
   return prepared;
}

}
