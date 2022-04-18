#include <gtest/gtest.h>
#include "interpreter/FragmentGenerator.h"
#include "interpreter/FragmentCache.h"
#include "codegen/backend_c/BackendC.h"
#include "exec/InterruptableJob.h"

namespace inkfuse {

namespace {

TEST(test_fragmentizors, fragment_generation) {
   auto fragments = FragmentGenerator::build();
   // Has to depend on the global runtime.
   EXPECT_EQ(fragments->getIncludes().size(), 1);
   // And only have generated functions.
   EXPECT_EQ(fragments->getStructs().size(), 0);
   EXPECT_GE(fragments->getFunctions().size(), 5);

   // Generated code should be compileable.
   BackendC backend;
   auto program = backend.generate(*fragments);
   EXPECT_NO_THROW(program->dump());
   InterruptableJob interrupt;
   EXPECT_NO_THROW(program->compileToMachinecode(interrupt));
}

TEST(test_fragmentizors, fragment_cache) {

   auto& cache = FragmentCache::instance();
   auto fragments = FragmentGenerator::build();
   for (auto& fragment: fragments->getFunctions()) {
      // Try to get handles to every function.
      const auto& name = fragment->name;
      EXPECT_NE(nullptr, cache.getFragment(name));
   }
}

}

}
