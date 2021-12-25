#ifndef INKFUSE_TYPES_H
#define INKFUSE_TYPES_H

#include <functional>
#include <memory>
#include <cassert>

/// This file contains the central building blocks for the backing types within the InkFuse IR.
namespace inkfuse {

namespace IR {

    /// Base type.
    struct Type {

        /// Virtual base destructor.
        virtual ~Type() = default;

        /// Is this a certain IR type?
        template <typename Other>
        Other* is() const {
            return dynamic_cast<Other*>(this);
        }

        template <typename Other>
        const Other* isConst() const {
            return dynamic_cast<const Other*>(this);
        }

        /// Given that this is a certain type, do something.
        template <typename Other>
        void actionIf(std::function<void(const Other&)> fnc) const {
            if (const auto elem = this->template is<Other>(); elem) {
                fnc(*elem);
            }
        }


    };

    using TypePtr = std::unique_ptr<Type>;

    /// Base type for a signed integer.
    struct SignedInt : public Type {

        /// Get number of bytes.
        virtual unsigned numBytes() const = 0;

    };

    /// Signed integer type of a certain byte count.
    template <unsigned bytes>
    struct SignedIntSized : public SignedInt {

        /// We only support certain integer sizes.
        static_assert(bytes == 1 || bytes == 2 || bytes == 4 || bytes == 8);

        /// Get number of bytes.
        unsigned numBytes() const override {
            return bytes;
        }

    };

    /// Base type for an unsigned integer.
    struct UnsignedInt : public Type {

        /// Get number of bytes.
        virtual unsigned numBytes() const = 0;

    };

    /// Unsigned integer type of a certain byte count.
    template <unsigned bytes>
    struct UnsignedIntSized : public UnsignedInt {

        /// We only support certain integer sizes.
        static_assert(bytes == 1 || bytes == 2 || bytes == 4 || bytes == 8);

        /// Get number of bytes.
        unsigned numBytes() const override {
            return bytes;
        }

    };

    struct Bool : public Type {

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
