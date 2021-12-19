#ifndef INKFUSE_IR_H
#define INKFUSE_IR_H

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

    /// Central IR program which is a set of functions.
    struct Program {

    };

    /// IR concept for operator state through which the compiled code can refer.
    struct GlobalState {

    };

    /// IR function getting a set of parameters and returning a result.
    struct Function {

    };

    /// A block of statements within a function.
    struct Block {

    };

    /// IR expression returning some type when evaluated.
    struct Expression {

    };

    /// Function invocation expression, for example used to call functions on members of the global state
    /// of this query.
    struct InvokeFunction: public Expression {

    };

    /// IR Statement, either an assignment or control flow.
    struct Statement {

    };

    /// Assignment statement.
    struct StmtAssignment: public Statement {

    };

    /// If statement.
    struct StmtIf: public Statement {

    };

    /// While statement.
    struct StmtWhile: public Statement {

    };

} // namespace IR

} // namespace inkfuse

#endif //INKFUSE_IR_H
