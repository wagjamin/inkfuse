#include <gtest/gtest.h>
#include "sample_programs.h"

namespace inkfuse {

/// Minimal tests which simply test if the sample programs can be built.

TEST(test_ir, program_1) {
   auto program = test_helpers::program_1();
   EXPECT_EQ(1, program.getFunctions().size());
}

}
