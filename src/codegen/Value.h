#ifndef INKFUSE_VALUE_H
#define INKFUSE_VALUE_H

#include "codegen/Type.h"
#include <memory>

namespace inkfuse {

namespace IR {

/// Value of a certain type
struct Value {
   virtual TypeArc getType() const = 0;

   virtual bool supportsInlining() const { return true; }
   virtual std::string str() const { return ""; };

   virtual std::unique_ptr<Value> copy() = 0;

   /// Get a void* to the raw value that can be interpreted by the compiled code.
   /// This is needed for suboperators like the RuntimeExpressionSubop.
   virtual void* rawData() = 0;

   virtual ~Value() = default;
};

using ValuePtr = std::unique_ptr<Value>;

template <unsigned size>
struct UI : public Value {
   static_assert(size == 1 || size == 2 || size == 8 || size == 4 || size == 8);

   UI(uint64_t value_) : value(value_) {}

   static ValuePtr build(uint64_t value) {
      return std::make_unique<UI<size>>(value);
   };

   std::unique_ptr<Value> copy() override {
      return build(value);
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
      // TODO A bit nasty as this can also be a narrower int type that
      // is cased to void and then back to int in the runtime.
      return &value;
   }
};

template <unsigned size>
struct SI : public Value {
   static_assert(size == 1 || size == 2 || size == 4 || size == 8);

   int64_t value;

   static ValuePtr build(int64_t value) {
      return ValuePtr(new SI<size>(value));
   };

   std::unique_ptr<Value> copy() override {
      return build(value);
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
      // TODO A bit nasty as this can also be a narrower int type that
      // is cased to void and then back to int in the runtime.
      return &value;
   }

   private:
   explicit SI(int64_t value_) : value(value_) {}
};

struct F8 : public Value {
   double value;

   static ValuePtr build(double value) {
      return ValuePtr(new F8(value));
   };

   std::unique_ptr<Value> copy() override {
      return build(value);
   };

   int64_t getValue() const {
      return value;
   }

   std::string str() const override {
      return std::to_string(value);
   }

   TypeArc getType() const override {
      return std::make_unique<Float>(8);
   }

   void* rawData() override {
      return &value;
   }

   private:
   explicit F8(double value_) : value(value_) {}
};

struct DateVal : public Value {
   int32_t value;

   static ValuePtr build(int32_t value) {
      return ValuePtr(new DateVal(value));
   };

   std::unique_ptr<Value> copy() override {
      return build(value);
   };

   std::string str() const override {
      return std::to_string(value);
   }

   TypeArc getType() const override {
      return std::make_unique<Date>();
   }

   void* rawData() override {
      return &value;
   }

   private:
   explicit DateVal(int32_t value_) : value(value_) {}
};

struct StringVal : public Value {
   // The managed string.
   std::string value;
   // A char* to the backing string. Needed to return a char** in the resolved
   // type in the runtime.
   char* value_ptr;

   static ValuePtr build(std::string value) {
      return ValuePtr(new StringVal(std::move(value)));
   }

   TypeArc getType() const override {
      return IR::String::build();
   };

   std::string str() const override {
      // Produce a literal that can be used for codegen.
      return "\"" + value + "\"";
   };

   std::unique_ptr<Value> copy() override {
      return build(value);
   };

   void* rawData() override {
      // We need to return a char** - this is because the runtime uses char* as the type for a string.
      // But this means that the erased type is char**.
      return &value_ptr;
   }

   private:
   StringVal(std::string value) : value(std::move(value)), value_ptr(this->value.data()) {
   }
};

/// A list of strings.Can be extended to a general-purpose value list in the future.
struct StringList : public Value {
   struct StringListView {
      const char** start;
      uint64_t size;
   };

   static ValuePtr build(std::vector<std::string> strings_) {
      return ValuePtr(new StringList(std::move(strings_)));
   }

   TypeArc getType() const override {
      // Pointer behind which the actual `StringListView` hides.
      return IR::Pointer::build(IR::Char::build());
   };

   // String list cannot be inlined. It needs to stay an abstract char*.
   bool supportsInlining() const override { return false; };

   std::string str() const override {
      throw std::runtime_error("str() on StringList not implemented");
   };

   std::unique_ptr<Value> copy() override {
      return StringList::build(strings);
   };

   void* rawData() override {
      // Return the raw state that can be interpreted by the runtime.
      assert(raw_view.size < 1000);
      assert(erased_view == &raw_view);
      return &erased_view;
   }

   std::vector<std::string> strings;
   std::unique_ptr<const char*[]> raw_chars;
   StringListView raw_view;
   void* erased_view;

   private:
   StringList(std::vector<std::string> strings_) : strings(std::move(strings_)) {
      raw_chars = std::make_unique<const char*[]>(strings.size());
      for (size_t k = 0; k < strings.size(); ++k) {
         raw_chars[k] = strings[k].c_str();
      }
      raw_view = StringListView{
         .start = raw_chars.get(),
         .size = strings.size(),
      };
      erased_view = &raw_view;
   }
};

}

}

#endif //INKFUSE_VALUE_H
