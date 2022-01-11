#ifndef INKFUSE_EXPRESSION_H
#define INKFUSE_EXPRESSION_H

#include "codegen/Type.h"
#include "codegen/Value.h"
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

/// This file contains all the required expression for the InkFuse IR.
namespace inkfuse {

namespace IR {

/// IR Expr returning some type when evaluated. An expression can have children which are again expressions.
/// Expressions can then be wrapped within IR statements for e.g. variable assignments or control blocks.
struct Expr {
   public:
   /// Constructor taking a vector of child expressions.
   Expr(std::vector<std::unique_ptr<Expr>> children_, TypeArc type_) : children(std::move(children_)), type(std::move(type_)) {
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
struct DeclareStmt;
struct Stmt;

/// Basic variable reference expression.
struct VarRefExpr : public Expr {
   /// Create a new varialbe reference for the given statement,
   /// throws if the statement is not a declare statement.
   VarRefExpr(const Stmt& declaration_);
   static ExprPtr build(const Stmt& declaration_);

   /// Backing declaration statement.
   const DeclareStmt& declaration;
};

/// Basic constant expression.
struct ConstExpr : public Expr {
   /// Create a new constant expression based on the given value.
   ConstExpr(ValuePtr value_) : Expr({}, value_->getType()), value(std::move(value_)){};
   static ExprPtr build(ValuePtr value_);

   /// Backing value.
   ValuePtr value;
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
   SubstitutableParameter(std::string param_name_, TypeArc type_) : Expr({}, std::move(type_)), param_name(std::move(param_name_)){};

   static_assert(std::is_convertible<BackingType*, Type*>::value, "SubsitutableParameter must go over type.");

   /// Backing parameter name.
   std::string param_name;
};

/// Function invocation expression, for example used to call functions on members of the global state
/// of this query.
/// Child expressions are serving as function arguments.
struct InvokeFctExpr : public Expr {
   public:
   InvokeFctExpr(std::string function_name_, TypeArc type_, std::vector<ExprPtr> args) : Expr(std::move(args), std::move(type_)), function_name(std::move(function_name_)){};

   private:
   /// Backing function name to be invoked.
   std::string function_name;
};

/// Binary expression - only exists for convenience.
struct BinaryExpr : public Expr {
   BinaryExpr(TypeArc type_, ExprPtr child_l_, ExprPtr child_r_) : Expr(std::vector<ExprPtr>{}, std::move(type_)) {
      children.emplace_back(std::move(child_l_));
      children.emplace_back(std::move(child_r_));
   };
};

/// Arithmetic expression doing some form of computation.
struct ArithmeticExpr : public BinaryExpr {
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

   /// Opcode of this expression.
   Opcode code;

   /// Constructor, suitable result type is inferred from child types and opcode.
   ArithmeticExpr(ExprPtr child_l_, ExprPtr child_r_, Opcode code_) : BinaryExpr(child_l_->type, std::move(child_l_), std::move(child_r_)), code(code_){};

   static ExprPtr build(ExprPtr child_l_, ExprPtr child_r_, Opcode code_) {
      return std::make_unique<ArithmeticExpr>(std::move(child_l_), std::move(child_r_), code_);
   };
};

/// Unary expression - only exists for convenience.
struct UnaryExpr : public Expr {
   UnaryExpr(ExprPtr child_, TypeArc type_) : Expr(std::vector<ExprPtr>{}, std::move(type_)) {
      children.emplace_back(std::move(child_));
   };
};

/// Cast expression.
struct CastExpr : public UnaryExpr {
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
