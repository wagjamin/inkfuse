#include "codegen/backend_c/BackendC.h"
#include "sample_programs.h"
#include <gtest/gtest.h>

namespace inkfuse {

// Tests for the C backend

TEST(test_c_backend, program_1) {
   // Set up the program.
   IR::Program program("test_ir_prog1", true);
   auto ir_builder = program.getIRBuilder();
   test_helpers::program_1(ir_builder);

   // Set up the c-backend.
   BackendC backend;
   auto c_program = backend.generate(program);

   // Compile it.
   c_program->compileToMachinecode();

   // Get a handle to the function.
   ASSERT_FALSE(c_program->getFunction("doesntexist"));
   auto fct = reinterpret_cast<uint32_t (*)()>(c_program->getFunction("simple_fct_1"));
   ASSERT_TRUE(fct);
   ASSERT_EQ(42, fct());
}

TEST(test_c_backend, program_2) {
   IR::Program program("test_ir_prog2", true);
   auto ir_builder = program.getIRBuilder();
   test_helpers::program_2(ir_builder);

   // Set up the c-backend.
   BackendC backend;
   auto c_program = backend.generate(program);

   // Compile it.
   c_program->compileToMachinecode();

   // Get a handle to the function.
   ASSERT_FALSE(c_program->getFunction("doesntexist"));
   auto fct = reinterpret_cast<uint64_t (*)(uint64_t, uint64_t)>(c_program->getFunction("simple_fct_2"));
   ASSERT_TRUE(fct);
   ASSERT_EQ(21, fct(21, 21));
   ASSERT_EQ(0, fct(21, 0));
   ASSERT_EQ(0, fct(0, 21));
   ASSERT_EQ(42, fct(61, 2));
}

TEST(test_c_backend, program_3) {
   IR::Program program("test_ir_prog3", true);
   auto ir_builder = program.getIRBuilder();
   test_helpers::program_3(ir_builder);

   // Set up the c-backend.
   BackendC backend;
   auto c_program = backend.generate(program);

   // Compile it.
   c_program->compileToMachinecode();

   // Get a handle to the function.
   ASSERT_FALSE(c_program->getFunction("doesntexist"));
   auto fct = reinterpret_cast<uint64_t (*)(uint64_t)>(c_program->getFunction("simple_fct_2"));
   ASSERT_TRUE(fct);
   ASSERT_EQ(0, fct(0));
   ASSERT_EQ(1, fct(1));
   ASSERT_EQ(1, fct(2));
   ASSERT_EQ(2, fct(3));
   ASSERT_EQ(3, fct(4));
   ASSERT_EQ(5, fct(5));
   ASSERT_EQ(8, fct(6));
   ASSERT_EQ(13, fct(7));
   ASSERT_EQ(21, fct(8));
   ASSERT_EQ(34, fct(9));
}

}
