#include "codegen/Expression.h"
#include "codegen/IR.h"
#include "codegen/Statement.h"

namespace inkfuse {

namespace IR {

CastExpr::CastExpr(ExprPtr child_, TypeArc target_) : UnaryExpr(std::move(target_)) {
   children.push_back(std::move(child_));
   auto res = validateCastable(*(children[0]->type), *type);
   if (res == CastResult::Forbidden) {
      throw std::runtime_error("forbidden IR cast");
   }
}

ExprPtr VarRefExpr::build(const Stmt& declaration_) {
   return std::make_unique<VarRefExpr>(declaration_);
}

VarRefExpr::VarRefExpr(const Stmt& declaration_)
   : Expr(dynamic_cast<const DeclareStmt&>(declaration_).type), declaration(dynamic_cast<const DeclareStmt&>(declaration_)) {
}

ExprPtr ConstExpr::build(ValuePtr value_) {
   return std::make_unique<ConstExpr>(std::move(value_));
}

InvokeFctExpr::InvokeFctExpr(const Function& fct_, std::vector<ExprPtr> args_)
    : Expr(fct_.return_type), fct(fct_)
{
   children = std::move(args_);
}

ExprPtr InvokeFctExpr::build(const Function& fct_, std::vector<ExprPtr> args)
{
   return ExprPtr(new InvokeFctExpr(fct_, std::move(args)));
}

bool ArithmeticExpr::isComparison(Opcode code)
{
   if (code == Opcode::Eq
       || code == Opcode::Less
       || code == Opcode::LessEqual
       || code == Opcode::Greater
       || code == Opcode::GreaterEqual) {
      return true;
   }
   return false;
}

TypeArc ArithmeticExpr::deriveType(const Expr& child_l, const Expr& child_r, Opcode code)
{
   if (isComparison(code)) {
      return Bool::build();
   }
   if (code == Opcode::HashCombine) {
      assert(dynamic_cast<IR::UnsignedInt*>(child_l.type.get()));
      return UnsignedInt::build(8);
   }
   // TODO nasty hack for the time being.
   assert(child_l.type);
   return child_l.type;
}

TypeArc DerefExpr::deriveType(const Expr &child)
{
    auto expr = reinterpret_cast<Pointer*>(child.type.get());
    if (!expr) {
        throw std::runtime_error("Can only dereference a pointer type.");
    }
    // Return backing type to which we point.
    return expr->pointed_to;
}

TypeArc StructAccesExpr::deriveType(const Expr &child, const std::string &field_name)
{
    auto struct_ptr = reinterpret_cast<Struct*>(child.type.get());
    if (!struct_ptr) {
        throw std::runtime_error("Can only access members of a struct.");
    }
    for (auto& field : struct_ptr->fields) {
        if (field.name == field_name) {
            return field.type;
        }
    }
    throw std::runtime_error("Requested field does not exist.");
}

    ExprPtr StructAccesExpr::build(ExprPtr child_, std::string field_)
    {
        auto expr = reinterpret_cast<Pointer*>(child_->type.get());
        if (expr) {
            // Child is a pointer, dereference first.
            child_ = DerefExpr::build(std::move(child_));
        }
        // Try to build a struct access expression.
        return std::make_unique<StructAccesExpr>(std::move(child_), std::move(field_));
    }

    CastExpr::CastResult CastExpr::validateCastable(const Type& src, const Type& target) {
   /*
        if (auto cSrc = src.isConst<SignedInt>()) {
            if (auto cTarget = target.isConst<SignedInt>()) {
                return cTarget->numBytes() >= cSrc->numBytes() ? CastResult::Permitted : CastResult::Narrowing;
            } else if (target.isConst<UnsignedInt>()) {
                return CastResult::Narrowing;
            } else if (target.isConst<Bool>()) {
                return CastResult::Narrowing;
            }
        } else if (auto cSrc = src.isConst<UnsignedInt>()) {
            if (auto cTarget = target.isConst<UnsignedInt>()) {
                return cTarget->numBytes() >= cSrc->numBytes() ? CastResult::Permitted : CastResult::Narrowing;
            } else if (auto cTarget = src.isConst<SignedInt>()) {
                // Unsigned can be safely casted into signed int with at least one more byte.
                return cTarget->numBytes() > cSrc->numBytes() ? CastResult::Permitted : CastResult::Narrowing;
            } else if (target.isConst<Bool>()) {
                return CastResult::Narrowing;
            }
        } else if (src.isConst<Bool>()) {
            if (target.isConst<UnsignedInt>() || target.isConst<SignedInt>()) {
                return CastResult::Permitted;
            }
        }
         */
   // Everything not mentioned explicitly is allowed - sorry.
   return CastResult::Permitted;
}

}

}