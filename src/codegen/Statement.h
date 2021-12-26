#ifndef INKFUSE_STATEMENT_H
#define INKFUSE_STATEMENT_H

#include <string>
#include <memory>
#include <exception>
#include "codegen/IRHelpers.h"
#include "codegen/Expression.h"
#include "codegen/Types.h"

/// This file contains the basic statements within the InkFuse IR.
namespace inkfuse {

namespace IR {

    /// Forward declarations.
    struct Block;
    using BlockPtr = std::unique_ptr<Block>;

    /// Basic IR statement.
    struct Stmt : public IRConcept {

        /// Virtual base destructor.
        virtual ~Stmt() = default;
    };

    using StmtPtr = std::unique_ptr<Stmt>;

    /// Function invocation statement.
    struct InvokeFctStmt : public Stmt {

        InvokeFctStmt(ExprPtr invoke_fct_expr_): invoke_fct_expr(std::move(invoke_fct_expr_)) {
            if (!invoke_fct_expr->is<InvokeFctExpr>()) {
                throw std::runtime_error("cannot wrap non invoke expression in invoke statement");
            }
        }

    private:
        /// Backing expression for invoking the function.
        ExprPtr invoke_fct_expr;
    };

    /// Variable declaration statement.
    struct DeclareStmt : public Stmt {

        DeclareStmt(std::string name_, TypePtr type_): name(std::move(name_)), type(std::move(type_)) {}

    private:
        /// Variable name.
        std::string name;
        /// Variable type.
        TypePtr type;
    };

    /// Assignment statement.
    struct AssignmentStmt : public Stmt {
    private:
        /// Variable name to which to write.
        std::string var_name;
        /// Expression which should be assigned.
        ExprPtr expr;
    };

    /// If statement.
    struct IfStmt : public Stmt {

    private:
        /// Expression to evaluate within the if statement.
        ExprPtr expr;
        /// If block.
        BlockPtr if_block;
        /// Optional else block.
        BlockPtr else_block;
    };

    /// While statement.
    struct WhileStmt : public Stmt {
        /// Expression to evaluate to decide whether to enter the while loop.
        ExprPtr expr;
        /// Block within the while statement.
        BlockPtr if_block;
    };


}

}

#endif //INKFUSE_STATEMENT_H
