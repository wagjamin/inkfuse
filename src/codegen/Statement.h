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

        /// Backing expression for invoking the function.
        ExprPtr invoke_fct_expr;
    };

    /// Variable declaration statement.
    struct DeclareStmt : public Stmt {

        DeclareStmt(std::string name_, TypePtr type_): name(std::move(name_)), type(std::move(type_)) {}

        /// Variable name.
        std::string name;
        /// Variable type.
        TypePtr type;
    };

    /// Assignment statement.
    struct AssignmentStmt : public Stmt {

        AssignmentStmt(std::string var_name_, ExprPtr expr_): var_name(std::move(var_name_)), expr(std::move(expr)) {}

        /// Variable name to which to write.
        std::string var_name;
        /// Expression which should be assigned.
        ExprPtr expr;
    };

    /// If statement.
    struct IfStmt : public Stmt {

        IfStmt(ExprPtr expr_, BlockPtr if_block_, BlockPtr else_block_): expr(std::move(expr_)), if_block(std::move(if_block_)), else_block(std::move(else_block_)) {}

        /// Expression to evaluate within the if statement.
        ExprPtr expr;
        /// If block.
        BlockPtr if_block;
        /// Optional else block.
        BlockPtr else_block;
    };

    /// While statement.
    struct WhileStmt : public Stmt {

        WhileStmt(ExprPtr expr_, BlockPtr if_block_): expr(std::move(expr)), if_block(std::move(if_block_)) {}

        /// Expression to evaluate to decide whether to enter the while loop.
        ExprPtr expr;
        /// Block within the while statement.
        BlockPtr if_block;
    };

    /// Statement visitor utility.
    template <typename Arg>
    struct StmtVisitor {
    public:
        void visit(const Stmt& stmt, Arg arg) {
            if (const auto elem = stmt.isConst<InvokeFctStmt>()) {
                visitInvokeFct(*elem, arg);
            } else if (auto elem = stmt.isConst<DeclareStmt>()) {
                visitDeclare(*elem, arg);
            } else if (auto elem = stmt.isConst<AssignmentStmt>()) {
                visitAssignment(*elem, arg);
            } else if (auto elem = stmt.isConst<IfStmt>()) {
                visitIf(*elem, arg);
            } else if (auto elem = stmt.isConst<WhileStmt>()) {
                visitWhile(*elem, arg);
            } else {
                assert(false);
            }
        }

    private:
        virtual void visitInvokeFct(const InvokeFctStmt& type, Arg arg) {}

        virtual void visitDeclare(const DeclareStmt& type, Arg arg) {}

        virtual void visitAssignment(const AssignmentStmt& type, Arg arg) {}

        virtual void visitIf(const IfStmt& type, Arg arg) {}

        virtual void visitWhile(const WhileStmt& type, Arg arg) {}
    };


}

}

#endif //INKFUSE_STATEMENT_H
