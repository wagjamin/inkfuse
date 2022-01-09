#ifndef INKFUSE_VALUE_H
#define INKFUSE_VALUE_H

#include "codegen/Type.h"
#include <memory>

namespace inkfuse {

namespace IR {

    /// Value of a certain type
    struct Value {

        virtual TypeArc getType() const = 0;

    };

    struct UI4: public Value {
        uint32_t value;

        UI4(uint32_t value_): value(value_) {}

        uint32_t getValue() const {
            return value;
        }

        TypeArc getType() const override {
            return std::make_unique<UnsignedInt>(4);
        }
    };
}

}

#endif //INKFUSE_VALUE_H
