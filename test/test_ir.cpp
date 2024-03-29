#include "sample_programs.h"
#include <gtest/gtest.h>

namespace inkfuse {

/// Minimal tests which simply test if the sample programs can be built.

TEST(test_ir, program_1) {
   IR::Program program("test_ir_prog1", true);
   auto ir_builder = program.getIRBuilder();
   test_helpers::program_1(ir_builder);
   EXPECT_EQ(1, program.getFunctions().size());
   EXPECT_EQ(0, program.getIncludes().size());
   EXPECT_EQ(0, program.getStructs().size());
}

TEST(test_ir, program_2) {
   IR::Program program("test_ir_prog2", true);
   auto ir_builder = program.getIRBuilder();
   test_helpers::program_2(ir_builder);
   EXPECT_EQ(1, program.getFunctions().size());
   EXPECT_EQ(0, program.getIncludes().size());
   EXPECT_EQ(0, program.getStructs().size());
}

TEST(test_ir, program_3) {
   IR::Program program("test_ir_prog3", true);
   auto ir_builder = program.getIRBuilder();
   test_helpers::program_3(ir_builder);
   EXPECT_EQ(1, program.getFunctions().size());
   EXPECT_EQ(0, program.getIncludes().size());
   EXPECT_EQ(0, program.getStructs().size());
}

TEST(test_ir, multiple_programs) {
   IR::Program program("test_ir_prog_multiple", true);
   auto ir_builder = program.getIRBuilder();
   test_helpers::program_1(ir_builder);
   test_helpers::program_2(ir_builder);
   EXPECT_EQ(2, program.getFunctions().size());
   EXPECT_EQ(0, program.getIncludes().size());
   EXPECT_EQ(0, program.getStructs().size());
}

TEST(test_ir, inkfuse_runtime) {
    IR::Program program("test_ir_inkfuse_runtime", false);
    EXPECT_EQ(1, program.getIncludes().size());
    const auto& include = program.getIncludes()[0];
    EXPECT_GE(include->getStructs().size(), 1);
}

}
