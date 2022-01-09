#ifndef INKFUSE_IR_H
#define INKFUSE_IR_H

#include "codegen/Statement.h"
#include "codegen/IRBuilder.h"
#include <utility>
#include <string>
#include <vector>

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

    struct Block;
    struct Program;
    using BlockPtr = std::unique_ptr<Block>;
    /// Programs can be referenced in many places. The best example is the global pre-amble
    /// provided by the runtime system containing runtime structs and external functions.
    using ProgramArc = std::shared_ptr<Program>;

    /// Global program containing all structs and functions exposed by the runtime system.
    extern ProgramArc global_runtime;

    /// IR function getting a set of parameters and returning a result.
    struct Function {

        Function(std::string name_, std::vector<TypeArc> arguments_);

        /// The unique function name.
        std::string name;

        /// The arguments.
        std::vector<TypeArc> arguments;

        /// Get the function body (if it exists). Note that code can only be generated through the IRBuilder.
        const Block* getBody();

    private:
        friend class FunctionBuilder;

        /// Optional function body. If this is not set, we assume the function is external
        /// and will be provided during linking. This is needed for runtime functions provided by
        /// e.g. hash tables.
        BlockPtr body;
    };

    using FunctionArc = std::shared_ptr<Function>;

    /// A block of statements within a function.
    struct Block {

        Block(std::vector<StmtPtr> statements_): statements(std::move(statements_)) {};

        /// Statements within the block.
        std::vector<StmtPtr> statements;
    };

    /// Central IR program which is a set of functions and structs.
    /// Cannot be used to generate code directly, this must go through the IRBuilder class.
    struct Program {

        /// Constructor, you have to provide a name for the program.
        /// If standalone is set, the global runtime definitions are not included by default (testing).
        Program(std::string program_name_, bool standalone = false);

        /// Get an IR builder for this program.
        IRBuilder getIRBuilder();

        /// The program name - leads to a globally unique path on the file system.
        const std::string program_name;

        const std::vector<ProgramArc>& getIncludes() const;

        const std::vector<StructArc>& getStructs() const;

        const std::vector<FunctionArc>& getFunctions() const;

    private:
        friend class IRBuilder;

        /// Other programs this program depends on. Usually only the global runtime.
        std::vector<ProgramArc> includes;

        /// A set of structs defined within the program.
        std::vector<StructArc> structs;

        /// A set of functions defined within the program.
        std::vector<FunctionArc> functions;
    };

} // namespace IR

} // namespace inkfuse

#endif //INKFUSE_IR_H
