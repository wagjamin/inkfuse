#include "exec/PipelineExecutor.h"
#include "algebra/CompilationContext.h"
#include "codegen/backend_c/BackendC.h"

namespace inkfuse {

uint64_t executor_id = 0;

PipelineExecutor::PipelineExecutor(const Pipeline& pipe_, std::string name_) : pipe(pipe_), context(pipe), name(std::move(name_))
{
   assert(pipe.suboperators[0]->isSource());
   assert(pipe.suboperators.back()->isSink());
   if (name.empty()) {
      name = "pipeline_" + std::to_string(executor_id++);
   }
}

void PipelineExecutor::run()
{
   for (auto& op: pipe.suboperators) {
      op->setUpState(context);
   }
   runFused();
   for (auto& op: pipe.suboperators) {
      op->tearDownState();
   }
}

void PipelineExecutor::runFused()
{
   // Create IR program for the pipeline.
   CompilationContext comp(name, pipe);
   comp.compile();

   // Generate C code.
   BackendC backend;
   auto program = backend.generate(comp.getProgram());
   program->compileToMachinecode();
   fct = reinterpret_cast<uint8_t(*)(void**, void**, void*)>(program->getFunction("execute"));

   while(runFusedMorsel()) {};
}

void PipelineExecutor::runInterpreted()
{
   // TODO
   throw std::runtime_error("not implemented");
}

void** PipelineExecutor::setUpState(size_t start, size_t end)
{
   std::vector<void*> res;
   res.reserve(pipe.suboperators.size());
   for (const auto& op: pipe.suboperators) {
      res.push_back(op->accessState());
   }
   auto& vec = states[{start, end}];
   vec = std::move(res);
   return vec.data();
}

bool PipelineExecutor::runFusedMorsel()
{
   if (!pipe.suboperators[0]->pickMorsel()) {
      return false;
   }
   auto state = setUpState(0, pipe.suboperators.size());
   for (size_t k = 0; k < pipe.scopes.size(); ++k) {
      context.clear(k);
   }
   fct(state, nullptr, nullptr);
   return true;
}

const ExecutionContext& PipelineExecutor::getExecutionContext()
{
   return context;
}

}
