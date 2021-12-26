#include "codegen/Expression.h"

namespace inkfuse {

namespace IR {

    const Expr &Expr::getChild(size_t idx) const
    {
        if (children.size() <= idx) {
            throw std::runtime_error("No child at index " + std::to_string(idx));
        }
        return *children[idx];
    }

    CastExpr::CastExpr(ExprPtr child, TypePtr target_, bool narrowing):
            UnaryExpr(std::move(child)) , target(std::move(target_))
    {
        assert(target);
        auto res = validateCastable(child->getType(), *target);
        if (res == CastResult::Forbidden || (!narrowing && res == CastResult::Narrowing)) {
            throw std::runtime_error("forbidden IR cast");
        }
    }

    CastExpr::CastResult CastExpr::validateCastable(const Type &src, const Type &target)
    {
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
        // Everything not mentioned explicitly is forbidden - sorry.
        return CastResult::Forbidden;
    }

}

}