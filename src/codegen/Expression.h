#ifndef INKFUSE_EXPRESSION_H
#define INKFUSE_EXPRESSION_H

#include <string>
#include <memory>
#include <vector>
#include <stdexcept>
#include "codegen/IRHelpers.h"
#include "codegen/Types.h"

/// This file contains all the required expression for the InkFuse IR.
namespace inkfuse {

namespace IR {

    /// IR Expr returning some type when evaluated. An expression can have children which are again expressions.
    /// Expressions can then be wrapped within IR statements for e.g. variable assignments or control blocks.
    struct Expr : public IRConcept {
    public:
        /// Constructor taking a vector of child expressions.
        Expr(std::vector<std::unique_ptr<Expr>> children_): children(std::move(children_)) {};

        /// Virtual base destructor.
        virtual ~Expr() = default;

        /// Get the return type of this expression.
        virtual const Type& getType() const;

        /// Get child expression at a certain index.
        const Expr& getChild(size_t idx) const;

    private:
        std::vector<std::unique_ptr<Expr>> children;
    };

    using ExprPtr = std::unique_ptr<Expr>;

    /// Basic variable reference expression.
    struct VarRefExpr: public Expr {
    public:
        VarRefExpr(std::string var_name_, TypePtr var_type_): Expr({}), var_name(std::move(var_name_)), var_type(std::move(var_type_)) {};

        const Type& getType() const override {
            return *var_type;
        }

    private:
        /// Backing variable name.
        std::string var_name;
        /// Type of the given variable.
        TypePtr var_type;
    };

    /// Basic constant expression.
    struct ConstExpr: public Expr {
    public:
        ConstExpr(std::string value_str_, TypePtr value_type_): Expr({}), value_str(std::move(value_str_)), value_type(std::move(value_type_)) {};

        const Type& getType() const override {
            return *value_type;
        }

    private:
        /// String describing the constant - nasty hack for the time being.
        /// Getting around this requires a decent amount of work though, check what NoisePage does.
        /// You effectively have to lower the parsed representation from your high-level query AST to a constant
        /// in your compiled language. Not particularly exciting, but important in a production system.
        std::string value_str;
        /// Type of the constant
        TypePtr value_type;
    };

    /// Function invocation expression, for example used to call functions on members of the global state
    /// of this query.
    /// Child expressions are serving as function arguments.
    struct InvokeFctExpr: public Expr {
    public:
        InvokeFctExpr(std::string functionName_, TypePtr type_, std::vector<ExprPtr> args): Expr(std::move(args)), functionName(std::move(functionName_)), type(std::move(type_)) {};

        const Type& getType() const override {
            return *type;
        }

    private:
        /// Backing function name to be invoked.
        std::string functionName;
        /// Return type of this function.
        TypePtr type;
    };

    /// Binary expression - only exists for convenience.
    struct BinaryExpr : public Expr {
        BinaryExpr(ExprPtr childL, ExprPtr childR): Expr(std::vector<ExprPtr>{std::move(childL), std::move(childR)}) {};
    };

    /// Arithmetic expression doing some form of computation.
    struct ArithmeticExpr: public BinaryExpr {

        /// Basic opcodes supported by an arithmetic expression.
        enum class Opcode {
            Add,
            Subtract,
            Multiply,
            Divide,
            Eq,
            Less,
            LessEqual,
            Greater,
            GreaterEqual,
        };

        /// Constructor providing custom result type for e.g. widening.
        ArithmeticExpr(ExprPtr childL, ExprPtr childR, TypePtr resultType);

        /// Constructor trying to take a suitable result type.
        ArithmeticExpr(ExprPtr childL, ExprPtr childR);
    };

    /// Unary expression - only exists for convenience.
    struct UnaryExpr : public Expr {
        UnaryExpr(ExprPtr child): Expr(std::vector<ExprPtr>{std::move(child)}) {};
    };

    /// Cast expression.
    struct CastExpr: public UnaryExpr {
    public:
        CastExpr(ExprPtr child, TypePtr target_, bool narrowing = true);

        const Type& getType() const override {
            return *target;
        }

        /// Possible results for casting.
        enum class CastResult {
            /// Cast is permitted.
            Permitted,
            /// Cast is permitted but might be narrowing.
            Narrowing,
            /// Cast is forbidden.
            Forbidden,
        };

        /// Validate that a type can be cast into another.
        static CastResult validateCastable(const Type& src, const Type& target);

    private:
        TypePtr target;
    };
}

}

#endif //INKFUSE_EXPRESSION_H
