#include "codegen/IRBuilder.h"
#include "codegen/IR.h"
#include <memory>

namespace inkfuse {

namespace IR {

    FunctionBuilder::FunctionBuilder(IRBuilder& builder_, FunctionArc function)
    : function(std::move(function)), builder(builder_)
    {
        if (function->body) {
            throw std::runtime_error("Cannot create function builder on function with non-empty body");
        }
        function->body = std::make_unique<Block>(std::vector<StmtPtr>{});
        curr_block = function->body.get();
    }

    void FunctionBuilder::appendStmt(StmtPtr stmt)
    {
        // Add a statement to the current block - note that this can be re-scoped by control flow blocks
        // like if statements and while loops.
        curr_block->statements.push_back(std::move(stmt));
    }

    FunctionBuilder::~FunctionBuilder()
    {
        finalize();
    }

    void FunctionBuilder::finalize()
    {
        if (!finalized) {
            // Add the function to the backing IR program.
            finalized = true;
            builder.addFunction(std::move(function));
        }
    }

    IRBuilder::IRBuilder(Program& program_): program(program_)
    {
    }

    void IRBuilder::addStruct(StructArc new_struct)
    {
        program.structs.push_back(std::move(new_struct));
    }

    void IRBuilder::addFunction(FunctionArc function)
    {
        program.functions.push_back(std::move(function));
    }

    FunctionBuilder IRBuilder::createFunctionBuilder(FunctionArc function)
    {
        return FunctionBuilder{*this, std::move(function)};
    }


}

}
