#include <gtest/gtest.h>
#include "interpreter/FragmentGenerator.h"
#include "codegen/backend_c/BackendC.h"

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
   EXPECT_NO_THROW(program->compileToMachinecode());
}

TEST(test_fragmentizors, fragment_execution) {

}

}

}
