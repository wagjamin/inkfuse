#include "codegen/Expression.h"
#include "codegen/Statement.h"

namespace inkfuse {

namespace IR {

CastExpr::CastExpr(ExprPtr child_, TypeArc target_, bool narrowing) : UnaryExpr(std::move(child_), std::move(target_)) {
   auto res = validateCastable(*(children[0]->type), *type);
   if (res == CastResult::Forbidden || (!narrowing && res == CastResult::Narrowing)) {
      throw std::runtime_error("forbidden IR cast");
   }
}

ExprPtr VarRefExpr::build(const Stmt& declaration_) {
   return std::make_unique<VarRefExpr>(declaration_);
}

VarRefExpr::VarRefExpr(const Stmt& declaration_)
   : Expr({}, dynamic_cast<const DeclareStmt&>(declaration_).type), declaration(dynamic_cast<const DeclareStmt&>(declaration_)) {
}

ExprPtr ConstExpr::build(ValuePtr value_) {
   return std::make_unique<ConstExpr>(std::move(value_));
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
      return Bool::getTypePtr();
   }
   // TODO nasty hack for the time being.
   return child_l.type;
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
   // Everything not mentioned explicitly is forbidden - sorry.
   return CastResult::Permitted;
}

}

}