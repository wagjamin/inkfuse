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
inline void program_1(IR::IRBuilder ir_builder)
{
   // Simple function with no arguments.
   auto fct_description = std::make_shared<IR::Function>(IR::Function("simple_fct_1", {}, IR::UnsignedInt::getTypePtr(4)));
   auto fct_builder = ir_builder.createFunctionBuilder(std::move(fct_description));

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
}

// Simple program adding two constant integers it gets as parameters and subtracting 21.
inline void program_2(IR::IRBuilder ir_builder)
{
   // Function takes two UI8s as arguments.
   std::vector<StmtPtr> arguments;
   arguments.push_back(DeclareStmt::build("ui1", UnsignedInt::getTypePtr(8)));
   arguments.push_back(DeclareStmt::build("ui2", UnsignedInt::getTypePtr(8)));
   auto fct_description = std::make_shared<IR::Function>(IR::Function("simple_fct_2", std::move(arguments), IR::UnsignedInt::getTypePtr(4)));
   auto fct_builder = ir_builder.createFunctionBuilder(std::move(fct_description));

   // Add the two parameters and store them in an intermediate.
   auto var_1 = DeclareStmt::build("intermediate", UnsignedInt::getTypePtr(8));

   // First get references to argument.
   auto arg_ref_1 = VarRefExpr::build(fct_builder.getArg(0));
   auto arg_ref_2 = VarRefExpr::build(fct_builder.getArg(1));
   // Add them
   auto add = ArithmeticExpr::build(std::move(arg_ref_1), std::move(arg_ref_2), ArithmeticExpr::Opcode::Add);
   // And assign.
   auto assign_1 = AssignmentStmt::build(*var_1, std::move(add));

   // Create a constant int.
   auto const_1 = ConstExpr::build(UI<8>::build(21));

   // Substract the constnat from the intermediate and return.
   auto intermediate_ref = VarRefExpr::build(*var_1);
   auto subtract = ArithmeticExpr::build(std::move(intermediate_ref), std::move(const_1), ArithmeticExpr::Opcode::Subtract);
   // And return.
   auto return_stmt = ReturnStmt::build(std::move(subtract));

   fct_builder.appendStmt(std::move(var_1));
   fct_builder.appendStmt(std::move(assign_1));
   fct_builder.appendStmt(std::move(return_stmt));

   // Wrap up the function builder and return.
   fct_builder.finalize();
}

}

}

#endif //INKFUSE_SAMPLE_PROGRAMS_H
