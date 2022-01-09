#include "codegen/IR.h"
#include "codegen/Type.h"
#include "codegen/Expression.h"
#include "codegen/Value.h"

#ifndef INKFUSE_SAMPLE_PROGRAMS_H
#define INKFUSE_SAMPLE_PROGRAMS_H

namespace inkfuse {

namespace test_helpers {

using namespace IR;

// Simple program adding two constant integers and returning them.
IR::Program program_1()
{
   IR::Program program("test", true);
   auto ir_builder = program.getIRBuilder();

   // Simple function with no arguments.
   auto fct_description = std::make_shared<IR::Function>(IR::Function("simple_fct", {}, IR::UnsignedInt::getTypePtr(4)));
   auto fct_builder = ir_builder.createFunctionBuilder(fct_description);

   // Create two constant ints.
   auto const_1 = ConstExpr::build(UI<4>::build(21));
   auto const_2 = ConstExpr::build(UI<4>::build(21));

   // Add them up.
   auto added = ArithmeticExpr::build(std::move(const_1), std::move(const_2), ArithmeticExpr::Opcode::Add);

   // And return.
   auto return_stmt = ReturnStmt::build(std::move(added));

   // Add the statement to the program.
   fct_builder.appendStmt(std::move(return_stmt));

   // Wrap up the function builder and return.
   fct_builder.finalize();

   return program;
}

}

}

#endif //INKFUSE_SAMPLE_PROGRAMS_H
