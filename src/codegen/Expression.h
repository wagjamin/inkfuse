#ifndef INKFUSE_EXPRESSION_H
#define INKFUSE_EXPRESSION_H

#include <memory>
#include <vector>
#include "codegen/Types.h"

/// This file contains all the required expression for the InkFuse IR.
namespace inkfuse {

namespace IR {

    /// IR Expression returning some type when evaluated. An expression is itself a hierarchy of expressions.
    struct Expression {
    public:
        /// Constructor taking a vector of child expressions.
        Expression(std::vector<std::unique_ptr<Expression>> children_): children(std::move(children_)) {};

        /// Virtual base destructor.
        virtual ~Expression() = default;

        /// Get the return type of this expression.
        virtual const Type& getType() const;

    private:
        std::vector<std::unique_ptr<Expression>> children;
    };

    /// Function invocation expression, for example used to call functions on members of the global state
    /// of this query.
    /// Child expressions are serving as function arguments.
    struct InvokeFunction: public Expression {


    private:
        /// Backing function name to be invoked.
        std::string functionName;
        /// Return type of this function.
        TypePtr type;
    };

    /// Cast expression.
    struct CastExpression: public Expression {

    private:
        TypePtr target;
    };

    /// Basic variable reference expression.
    struct VarRefExpression: public Expression {

    };

    /// Basic constant expression.
    struct ConstExpression: public Expression {

    };

    using ExpressionPtr = std::unique_ptr<Expression>;

}

}

#endif //INKFUSE_EXPRESSION_H
