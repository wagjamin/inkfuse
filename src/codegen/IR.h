#ifndef INKFUSE_IR_H
#define INKFUSE_IR_H

#include "codegen/Statement.h"

/// This file contains all the central building blocks for the InkFuse IR.
/// The purpose of the InkFuse IR is to make generating code easier and support different compiler backends.
/// It allows for the creation of a high-level (C-like) IR in SSA form.
///
/// This is just a toy IR compared to some of the real DB IRs out there. A good example is the NoisePage IR
/// developed at CMU - take a look!
///
/// This IR can be later lowered to C. If we find that C is not performant enough wrt. compilation time
/// it will allow us to switch to LLVM codegen without changing the actual code generation logic.
namespace inkfuse {

namespace IR {

    /// IR function getting a set of parameters and returning a result.
    struct Function {

    private:
        /// The unique function name.
        std::string name;

        /// The arguments.
        std::vector<TypePtr> arguments;
    };

    using FunctionPtr = std::unique_ptr<Function>;

    /// A block of statements within a function.
    struct Block {
    public:

    private:
        /// Statements within the block.
        std::vector<StmtPtr> statements;
    };

    using BlockPtr = std::unique_ptr<Block>;

    /// TODO Make this a type?
    /// A struct of base types.
    struct Structure {

    };

    using StructurePtr = std::unique_ptr<Structure>;

    /// Central IR program which is a set of functions and structs.
    struct Program {
        /// A set of structs defined within the program.
        std::vector<StructurePtr> structures;

        /// A set of functions defined within the program.
        std::vector<FunctionPtr> functions;
    };


} // namespace IR

} // namespace inkfuse

#endif //INKFUSE_IR_H
