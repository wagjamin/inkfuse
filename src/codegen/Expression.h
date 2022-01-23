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
/// TODO - It would make to have some context of lvalues and rvalues here - but we can cut corners here.
/// Why? Because we are very careful in generating code, we can just assume every expression can be referenced.
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
   /// Create a new variable reference for the given statement,
   /// throws if the statement is not a declare statement.
   VarRefExpr(const Stmt& declaration_);

   /// Build a reference to the underlying variable.
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

struct Function;

/// Function invocation expression, for example used to call functions on members of the global state
/// of this query.
/// Child expressions are serving as function arguments.
struct InvokeFctExpr : public Expr {
   public:
   InvokeFctExpr(const Function& fct_, std::vector<ExprPtr> args);

   private:
   /// Backing function to be invoked.
   const Function& fct;
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

   /// Is the opcode one of a comparison?
   static bool isComparison(Opcode code);
   /// Derive the result type of an expression.
   static TypeArc deriveType(const Expr& child_l, const Expr& child_r, Opcode code);

   /// Constructor, suitable result type is inferred from child types and opcode.
   ArithmeticExpr(ExprPtr child_l_, ExprPtr child_r_, Opcode code_) : BinaryExpr(deriveType(*child_l_, *child_r_, code_), std::move(child_l_), std::move(child_r_)), code(code_){};

   static ExprPtr build(ExprPtr child_l_, ExprPtr child_r_, Opcode code_) {
      return std::make_unique<ArithmeticExpr>(std::move(child_l_), std::move(child_r_), code_);
   };
};

/// Unary expression - only exists for convenience.
struct UnaryExpr : public Expr {
   UnaryExpr(TypeArc type_, ExprPtr child_) : Expr(std::vector<ExprPtr>{}, std::move(type_)) {
      children.emplace_back(std::move(child_));
   };
};

/// Dereference a pointer. Child must be an expression with a pointer type.
struct DerefExpr : public UnaryExpr {

    DerefExpr(ExprPtr child_) : UnaryExpr(deriveType(*child_), std::move(child_)) {}

    /// Derive the result type of an expression.
    static TypeArc deriveType(const Expr& child);

    static ExprPtr build(ExprPtr child_) {
        return std::make_unique<DerefExpr>(std::move(child_));
    }

};

/// Access a member of a struct - return type is the type of the struct member.
/// The child expression must be typed a struct.
struct StructAccesExpr : public UnaryExpr {

    StructAccesExpr(ExprPtr child_, std::string field_): UnaryExpr(deriveType(*child_, field_), std::move(child_)), field(std::move(field_)) {};

    /// Derive the result type of accessing a struct member.
    static TypeArc deriveType(const Expr& child, const std::string& field);

    /// Build a struct access expression. Accepts a child expression of type struct or pointer
    /// to struct. Note that if the child is a pointer type, we add a deref expression below
    /// the struct access expression.
    static ExprPtr build(ExprPtr child_, std::string field_);

    /// Which field to access.
    std::string field;
};

/// Cast expression.
struct CastExpr : public UnaryExpr {
   public:
   CastExpr(ExprPtr child, TypeArc target_);

   static ExprPtr build(ExprPtr child, TypeArc target_) {
       return std::make_unique<CastExpr>(std::move(child), std::move(target_));
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
};

/// Expression visitor utility.
template <typename Arg>
struct ExprVisitor {
public:
    void visit(const Expr& expr, Arg arg) {
        if (const auto elem = dynamic_cast<const VarRefExpr*>(&expr)) {
            visitVarRef(*elem, arg);
        } else if (auto elem = dynamic_cast<const ConstExpr*>(&expr)) {
            visitConst(*elem, arg);
        } else if (auto elem = dynamic_cast<const InvokeFctExpr*>(&expr)) {
            visitInvokeFct(*elem, arg);
        } else if (auto elem = dynamic_cast<const ArithmeticExpr*>(&expr)) {
            visitArithmetic(*elem, arg);
        } else if (auto elem = dynamic_cast<const DerefExpr*>(&expr)) {
            visitDeref(*elem, arg);
        } else if (auto elem = dynamic_cast<const StructAccesExpr*>(&expr)) {
            visitStructAccess(*elem, arg);
        } else if (auto elem = dynamic_cast<const CastExpr*>(&expr)) {
            visitCast(*elem, arg);
        } else {
            assert(false);
        }
    }

private:
    virtual void visitVarRef(const VarRefExpr& type, Arg arg) { }

    virtual void visitConst(const ConstExpr& type, Arg arg) { }

    virtual void visitInvokeFct(const InvokeFctExpr& type, Arg arg) { }

    virtual void visitArithmetic(const ArithmeticExpr& type, Arg arg) { }

    virtual void visitDeref(const DerefExpr& type, Arg arg) { }

    virtual void visitStructAccess(const StructAccesExpr& type, Arg arg) { }

    virtual void visitCast(const CastExpr& type, Arg arg) { }

};


}

}

#endif //INKFUSE_EXPRESSION_H
