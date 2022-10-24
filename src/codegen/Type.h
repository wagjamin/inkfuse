#ifndef INKFUSE_TYPE_H
#define INKFUSE_TYPE_H

#include <cassert>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

/// This file contains the central building blocks for the backing types within the InkFuse IR.
namespace inkfuse {

namespace IR {

/// Base type.
struct Type {
   /// Virtual base destructor.
   virtual ~Type() = default;

   /// Given that this is a certain type, do something.
   template <typename Other>
   void actionIf(std::function<void(const Other&)> fnc) const {
      if (const auto elem = dynamic_cast<Other*>(this); elem) {
         fnc(*elem);
      }
   }

   /// Get the size of this type in bytes (i.e. how much memory it consumes).
   virtual size_t numBytes() const = 0;

   /// Get a string representation of this type.
   virtual std::string id() const = 0;

   /// Write a value of the type .
   virtual void print(std::ostream& stream, char* data) const {
      throw std::runtime_error("Type cannot be printed");
   };

   bool operator==(const Type& other) const {
      /// Simple equality check for types which is actually sufficient for our type system.
      return id() == other.id();
   }
};

/// Types are shared pointers. This is because struct types can get pretty complicated and we don't want
/// to copy all the time. Note that there might be tons of expressions referencing the struct type.
using TypeArc = std::shared_ptr<Type>;

/// A type which also exists in SQL (e.g. integers, strings, ...)
struct SQLType : public Type {};

/// Base type for a signed integer.
struct SignedInt : public SQLType {
   SignedInt(size_t size_) : size(size_){};

   static TypeArc build(size_t size) {
      return std::make_shared<SignedInt>(size);
   };

   size_t numBytes() const override {
      return size;
   };

   std::string id() const override {
      return "I" + std::to_string(size);
   };

   void print(std::ostream& stream, char* data) const override;

   private:
   uint64_t size;
};

/// Base type for an unsigned integer.
struct UnsignedInt : public SQLType {
   UnsignedInt(size_t size_) : size(size_){};

   static TypeArc build(size_t size) {
      return std::make_shared<UnsignedInt>(size);
   };

   size_t numBytes() const override {
      return size;
   };

   std::string id() const override {
      return "UI" + std::to_string(size);
   };

   void print(std::ostream& stream, char* data) const override;

   private:
   uint64_t size;
};

struct Float : public SQLType {
   Float(size_t size_) : size(size_) {
      assert(size == 4 || size == 8);
   }

   static TypeArc build(size_t size) {
      return std::make_shared<Float>(size);
   };

   size_t numBytes() const override {
      return size;
   };

   std::string id() const override {
      return "F" + std::to_string(size);
   }

   void print(std::ostream& stream, char* data) const override;

   private:
   uint64_t size;
};

/// Boolean type.
struct Bool : public SQLType {
   static TypeArc build() {
      return std::make_shared<Bool>();
   };

   size_t numBytes() const override {
      return 1;
   };

   std::string id() const override {
      return "Bool";
   };

   void print(std::ostream& stream, char* data) const override;
};

/// Character type.
struct Char : public SQLType {
   static TypeArc build() {
      return std::make_shared<Char>();
   };

   size_t numBytes() const override {
      return 1;
   }

   std::string id() const override {
      return "Char";
   }

   void print(std::ostream& stream, char* data) const override;
};

/// Text type. In the engine it is represented as a char* to a 0-terminated string.
struct String : public SQLType {
   static TypeArc build() {
      return std::make_shared<String>();
   }

   size_t numBytes() const override {
      return 8;
   }

   std::string id() const override {
      return "String";
   }

   void print(std::ostream& stream, char* data) const override;
};

/// Date type.
struct Date : public SQLType {
   static TypeArc build() {
      return std::make_shared<Date>();
   }

   size_t numBytes() const override {
      return 4;
   }

   std::string id() const override {
      return "Date";
   }
};

/// Void type which is usually wrapped into pointers for a lack of better options.
struct Void : public Type {
   static TypeArc build() {
      return std::make_unique<Void>();
   };

   size_t numBytes() const override {
      return 0;
   }

   std::string id() const override {
      return "Void";
   }
};

/// A statically sized byte array. Can be used e.g. as intermediate state for key packing.
struct ByteArray : public Type {
   static TypeArc build(size_t size_) {
      return std::make_shared<ByteArray>(size_);
   }
   ByteArray(size_t size_) : size(size_) {}

   size_t numBytes() const override {
      return size;
   }

   std::string id() const override {
      // Width is not part of type id as we want one primitive for all sizes.
      return "ByteArray";
   }

   private:
   size_t size;
};

/// Custom struct type, this is needed to make the IR aware of C-style structs
/// containing operator state.
struct Struct : public Type {
   /// A field within the struct.
   struct Field {
      /// Type of the field.
      TypeArc type;
      /// Name of the field.
      std::string name;
   };

   Struct(std::string name_, std::vector<Field> fields_) : name(std::move(name_)), fields(std::move(fields_)) {}

   static TypeArc build(std::string name_, std::vector<Field> fields_) {
      return std::make_unique<Struct>(std::move(name_), std::move(fields_));
   }

   /// Struct name.
   std::string name;
   /// The fields stored within the struct (in the given order!).
   /// Note - the NoisePage IR also stores offsets of struct members in their IR structs.
   /// Why don't we need this? Because we generate C/C++ ;-)
   /// If you generate LLVM/ASM directly you need to explicitly calculate offsets etc. when you access C-style
   /// structs, this is a responsibility we move to the C compiler in this case, we can just access through
   /// regular "dot-syntax".
   std::vector<Field> fields;

   size_t numBytes() const override {
      // TODO - I hope we won't need this ... a note on this:
      // We would actually need it 100% if we put the global state of operators consecutively.
      // In our world where we need to switch between vectorization and compilation however this is tricky.
      // We need the state in different places depending on whether we interpret or fuse.
      // In that case it's actually easier to represent the global state as a vector of pointers to the backing
      // operator state.
      throw std::runtime_error("not implemented");
   }

   std::string id() const override {
      return "Struct_" + name;
   }
};

using StructArc = std::shared_ptr<Struct>;

/// Pointer to some other type.
struct Pointer : public Type {
   /// Type this pointer points to.
   TypeArc pointed_to;

   Pointer(TypeArc pointed_to_) : pointed_to(std::move(pointed_to_)) {
   }

   static TypeArc build(TypeArc pointed_to) {
      return std::make_unique<Pointer>(pointed_to);
   };

   size_t numBytes() const override {
      return 8;
   }

   std::string id() const override {
      return "Ptr_" + pointed_to->id();
   }
};

/// Type visitor utility.
template <typename Arg>
struct TypeVisitor {
   public:
   void visit(const IR::Type& type, Arg arg) {
      if (const auto elem = dynamic_cast<const IR::SignedInt*>(&type)) {
         visitSignedInt(*elem, arg);
      } else if (auto elem = dynamic_cast<const IR::UnsignedInt*>(&type)) {
         visitUnsignedInt(*elem, arg);
      } else if (auto elem = dynamic_cast<const IR::Float*>(&type)) {
         visitFloat(*elem, arg);
      } else if (auto elem = dynamic_cast<const IR::Bool*>(&type)) {
         visitBool(*elem, arg);
      } else if (auto elem = dynamic_cast<const IR::Char*>(&type)) {
         visitChar(*elem, arg);
      } else if (auto elem = dynamic_cast<const IR::String*>(&type)) {
         visitString(*elem, arg);
      } else if (auto elem = dynamic_cast<const IR::Date*>(&type)) {
         visitDate(*elem, arg);
      } else if (auto elem = dynamic_cast<const IR::ByteArray*>(&type)) {
         visitByteArray(*elem, arg);
      } else if (auto elem = dynamic_cast<const IR::Void*>(&type)) {
         visitVoid(*elem, arg);
      } else if (auto elem = dynamic_cast<const IR::Struct*>(&type)) {
         visitStruct(*elem, arg);
      } else if (auto elem = dynamic_cast<const IR::Pointer*>(&type)) {
         visitPointer(*elem, arg);
      } else {
         assert(false);
      }
   }

   private:
   virtual void visitSignedInt(const SignedInt& type, Arg arg) {}

   virtual void visitUnsignedInt(const UnsignedInt& type, Arg arg) {}

   virtual void visitFloat(const Float& type, Arg arg) {}

   virtual void visitBool(const Bool& type, Arg arg) {}

   virtual void visitChar(const Char& type, Arg arg) {}

   virtual void visitString(const String& type, Arg arg) {}

   virtual void visitDate(const Date& type, Arg arg) {}

   virtual void visitByteArray(const ByteArray& type, Arg arg) {}

   virtual void visitVoid(const Void& type, Arg arg) {}

   virtual void visitStruct(const Struct& type, Arg arg) {}

   virtual void visitPointer(const Pointer& type, Arg arg) {}
};

}

}

#endif //INKFUSE_TYPE_H
