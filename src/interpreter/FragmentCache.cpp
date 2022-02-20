#include "interpreter/FragmentCache.h"
#include "interpreter/FragmentGenerator.h"
#include "codegen/backend_c/BackendC.h"

namespace inkfuse {

FragmentCache::FragmentCache()
{
   // Generate the fragments.
   auto fragments = FragmentGenerator::build();
   BackendC backend;
   program = backend.generate(*fragments);
   program->compileToMachinecode();
   // And link directly to now slow down the first queries.
   program->link();
}

void* FragmentCache::getFragment(std::string_view name)
{
   return program->getFunction(name);
}

}
