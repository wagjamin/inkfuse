#ifndef INKFUSE_VALUE_H
#define INKFUSE_VALUE_H

#include "codegen/Type.h"
#include <memory>

namespace inkfuse {

namespace IR {

/// Value of a certain type
struct Value {
   virtual TypeArc getType() const = 0;
   virtual std::string str() const { return ""; };

   /// Get a void* to the raw value that can be interpreted by the compiled code.
   /// This is needed for suboperators like the LazyExpressionSubop.
   virtual void* rawData() = 0;

   virtual ~Value() = default;
};

using ValuePtr = std::unique_ptr<Value>;

template <unsigned size>
struct UI : public Value {
   static_assert(size == 1 || size == 8 || size == 4 || size == 8);

   UI(uint64_t value_) : value(value_) {}

   static ValuePtr build(uint64_t value) {
      return std::make_unique<UI<size>>(value);
   };

   uint64_t value;

   uint64_t getValue() const {
      return value;
   }

   TypeArc getType() const override {
      return std::make_unique<UnsignedInt>(size);
   }

   std::string str() const override {
      return std::to_string(value);
   }

   void* rawData() override {
      return &value;
   }
};

template <unsigned size>
struct SI : public Value {
   static_assert(size == 1 || size == 2 || size == 4 || size == 8);

   int64_t value;

   static ValuePtr build(uint64_t value) {
      return std::make_unique<SI<size>>(value);
   };

   int64_t getValue() const {
      return value;
   }

   std::string str() const override {
      return std::to_string(value);
   }

   TypeArc getType() const override {
      return std::make_unique<SignedInt>(size);
   }

   void* rawData() override {
      return &value;
   }

   private:
   SI(int64_t value_) : value(value_) {}
};

}

}

#endif //INKFUSE_VALUE_H
