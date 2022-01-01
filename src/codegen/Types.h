#ifndef INKFUSE_TYPES_H
#define INKFUSE_TYPES_H

#include <functional>
#include <memory>
#include <cassert>
#include "codegen/IRHelpers.h"

/// This file contains the central building blocks for the backing types within the InkFuse IR.
namespace inkfuse {

namespace IR {

    /// Base type.
    struct Type : public IRConcept {

        /// Virtual base destructor.
        virtual ~Type() = default;

        /// Given that this is a certain type, do something.
        template <typename Other>
        void actionIf(std::function<void(const Other&)> fnc) const {
            if (const auto elem = this->template is<Other>(); elem) {
                fnc(*elem);
            }
        }

        /// Some form of maximum operation over two types which is supposed to find a suitable target type
        /// for arithmetic operations. But the maximum type is not safe of under- or overflow!
        static std::unique_ptr<Type> max(const Type& t1, const Type& t2);

        /// Get the size of this type in bytes (i.e. how much memory it consumes).
        virtual size_t numBytes() const = 0;
    };

    using TypePtr = std::unique_ptr<Type>;

    /// Base type for a signed integer.
    struct SignedInt : public Type {

        SignedInt(size_t size_): size(size_) {};

        size_t numBytes() const override {
            return size;
        };

    private:
        uint64_t size;
    };

    /// Base type for an unsigned integer.
    struct UnsignedInt : public Type {

        UnsignedInt(size_t size_): size(size_) {};

        size_t numBytes() const override {
            return size;
        };

    private:
        uint64_t size;
    };

    struct Bool : public Type {

        size_t numBytes() const override {
            return 1;
        };
    };

    /// Type visitor utility.
    template <typename Arg>
    struct TypeVisitor {
    public:
        void visit(const IR::Type& type, Arg arg) {
            if (const auto elem = type.isConst<IR::SignedInt>()) {
                visitSignedInt(*elem, arg);
            } else if (auto elem = type.isConst<IR::UnsignedInt>()) {
                visitUnsignedInt(*elem, arg);
            } else if (auto elem = type.isConst<IR::Bool>()) {
                visitBool(*elem, arg);
            } else {
                assert(false);
            }
        }

    private:
        virtual void visitSignedInt(const SignedInt& type, Arg arg) {}

        virtual void visitUnsignedInt(const UnsignedInt& type, Arg arg) {}

        virtual void visitBool(const Bool& type, Arg arg) {}
    };

}

}

#endif //INKFUSE_TYPES_H
