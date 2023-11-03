#include "interpreter/FragmentCache.h"
#include "interpreter/FragmentGenerator.h"
#include "codegen/backend_c/BackendC.h"
#include "exec/InterruptableJob.h"

namespace inkfuse {

FragmentCache::FragmentCache()
{
   // Generate the fragments.
   auto fragments = FragmentGenerator::build();
   BackendC backend;
   program = backend.generate(*fragments);
   InterruptableJob interrupt;
   program->compileToMachinecode(interrupt, /* compile_for_interpreter = */ true);
   // And link directly to now slow down the first queries.
   program->link();
}

void* FragmentCache::getFragment(std::string_view name)
{
   return program->getFunction(name);
}

}
