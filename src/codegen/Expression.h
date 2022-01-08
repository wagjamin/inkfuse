#ifndef INKFUSE_EXPRESSION_H
#define INKFUSE_EXPRESSION_H

#include <string>
#include <memory>
#include <vector>
#include <stdexcept>
#include <type_traits>
#include "codegen/IRHelpers.h"
#include "codegen/Type.h"

/// This file contains all the required expression for the InkFuse IR.
namespace inkfuse {

namespace IR {

    /// IR Expr returning some type when evaluated. An expression can have children which are again expressions.
    /// Expressions can then be wrapped within IR statements for e.g. variable assignments or control blocks.
    struct Expr : public IRConcept {
    public:
        /// Constructor taking a vector of child expressions.
        Expr(std::vector<std::unique_ptr<Expr>> children_, TypeArc type_): children(std::move(children_)), type(std::move(type_))
        {
            /// Every expression must have a type.
            assert(type);
        };

        /// Virtual base destructor.
        virtual ~Expr() = default;

        /// Children.
        std::vector<std::unique_ptr<Expr>> children;
        /// Result type.
        TypeArc type;
    };

    using ExprPtr = std::unique_ptr<Expr>;

    /// Basic variable reference expression.
    struct VarRefExpr: public Expr {
        VarRefExpr(std::string var_name_, TypeArc var_type_): Expr({}, std::move(var_type_)), var_name(std::move(var_name_)) {};

        /// Backing variable name.
        std::string var_name;
    };

    /// Basic constant expression.
    struct ConstExpr: public Expr {
    public:
        ConstExpr(std::string value_str_, TypeArc value_type_): Expr({}, std::move(value_type_)), value_str(std::move(value_str_)) {};

    private:
        /// String describing the constant - nasty hack for the time being.
        /// Getting around this requires a decent amount of work though, check what NoisePage does.
        /// You effectively have to lower the parsed representation from your high-level query AST to a constant
        /// in your compiled language. Not particularly exciting, but important in a production system.
        std::string value_str;
    };

    /// A substitutable parameter is at the heart of InkFuse. It effectively describes a value which during actual
    /// low-level code generation turns into a hard-coded constant, or a function parameter.
    /// The core challenge underpinning the InkFuse approach is that there has to be a limited degree of freedom
    /// in the vectorized primitives we are generating. At the same time, just in time compiled code has almost
    /// unlimited freedom coming for example from something like complex aggregation state in hash tables.
    ///
    /// A substitutable parameter which has a contained value gets hard-coded by the backend. A substitutable parameter which
    /// has no contained value meanwhile gets turned into a parameter by every function using it.
    /// The actual value maps are maintained by the runtime system and passed through the interfaces to compiled code which
    /// can then either choose to use a pre-created vectorized fragment with parameter subsitution, or a compiled one that
    /// does not need the parameter.
    template <typename BackingType>
    struct SubstitutableParameter : public Expr {
        SubstitutableParameter(std::string param_name_, TypeArc type_): Expr({}, std::move(type_)), param_name(std::move(param_name_)) {};

        static_assert(std::is_convertible<BackingType*, Type*>::value, "SubsitutableParameter must go over type.");

        /// Backing parameter name.
        std::string param_name;
    };

    /// Function invocation expression, for example used to call functions on members of the global state
    /// of this query.
    /// Child expressions are serving as function arguments.
    struct InvokeFctExpr: public Expr {
    public:
        InvokeFctExpr(std::string function_name_, TypeArc type_, std::vector<ExprPtr> args): Expr(std::move(args), std::move(type_)), function_name(std::move(function_name_)) {};

    private:
        /// Backing function name to be invoked.
        std::string function_name;
    };

    /// Binary expression - only exists for convenience.
    struct BinaryExpr : public Expr {
        BinaryExpr(ExprPtr child_l_, ExprPtr child_r_, TypeArc type_): Expr(std::vector<ExprPtr>{std::move(child_l_), std::move(child_r_)}, std::move(type_)) {};
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

        /// Constructor, suitable result type is inferred from child types and opcode.
        ArithmeticExpr(ExprPtr child_l_, ExprPtr child_r_, TypeArc type_, Opcode code_): BinaryExpr(std::move(child_l_), std::move(child_r_), std::move(type_)), code(code_) {};

        /// Opcode of this expression.
        Opcode code;
    };

    /// Unary expression - only exists for convenience.
    struct UnaryExpr : public Expr {
        UnaryExpr(ExprPtr child_, TypeArc type_): Expr(std::vector<ExprPtr>{std::move(child_)}, std::move(type_) {};
    };

    /// Cast expression.
    struct CastExpr: public UnaryExpr {
    public:
        CastExpr(ExprPtr child, TypeArc target_, bool narrowing = true);

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
    };
}

}

#endif //INKFUSE_EXPRESSION_H
