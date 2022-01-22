#include <gtest/gtest.h>
#include "algebra/ColumnScan.h"
#include "codegen/backend_c/BackendC.h"
#include "codegen/Expression.h"
#include "codegen/IR.h"
#include "codegen/Type.h"
#include "codegen/Value.h"


namespace inkfuse {

    /// Tests for whether the runtime can be integrated into the system.
    /// Created runtime structs and functions and checks if mutual calls are possible.

    /// Testing function runtime integration.
    TEST(test_runtime, runtime_extfunc) {



    }

    /// Testing struct runtime integration.
    TEST(test_runtime, runtime_extstruct) {

    }

    /// Complex runtime tests using both structs and functions.
    TEST(test_runtime, runtime_complex) {

    }

    /// Real inkfuse runtime with all operators and runtime structs attached.
    TEST(test_runtime, runtime_real) {

        // IR program which is not compiled in standalone mode - as a result, the runtime gets included.
        IR::Program program("test_runtime_test_real", false);

        auto ir_builder = program.getIRBuilder();
        auto& runtime = *program.getIncludes()[0];
        // Set up runtime parameter the new function will receive.
        auto arg = IR::DeclareStmt::build("runtime_param", IR::Pointer::build(runtime.getStruct("ColumnScanState")));
        std::vector<IR::StmtPtr> params;
        params.push_back(std::move(arg));
        // Function we are building.
        auto fct = std::make_shared<IR::Function>("test_fct", std::move(params), IR::UnsignedInt::build(8));
        auto fct_builder = ir_builder.createFunctionBuilder(std::move(fct));

        // Return the size.
        auto expr = IR::StructAccesExpr::build(IR::VarRefExpr::build(fct_builder.getArg(0)), "size");
        auto stmt = IR::ReturnStmt::build(std::move(expr));
        fct_builder.appendStmt(std::move(stmt));

        // Set up the c-backend.
        BackendC backend;
        auto c_program = backend.generate(program);

        // Compile it.
        c_program->compileToMachinecode();

        // Get a handle to the function.
        auto fct_compiled = reinterpret_cast<uint64_t (*)(ColumnScanState*)>(c_program->getFunction("test_fct"));

        for (uint64_t i : {1, 100, 1000, 42, 311}) {
            // Set up actual parameter.
            ColumnScanState param {
                    .data = nullptr,
                    .sel = nullptr,
                    .size = i,
            };

            // Invoke the test function multiple times.
            ASSERT_EQ(i, fct_compiled(&param));
        }

    }

}
