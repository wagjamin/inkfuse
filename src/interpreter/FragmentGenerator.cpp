#include "interpreter/FragmentGenerator.h"
#include "algebra/CompilationContext.h"
#include "interpreter/AggregationFragmentizer.h"
#include "interpreter/ColumnFilterFragmentizer.h"
#include "interpreter/CopyFragmentizer.h"
#include "interpreter/CountingSinkFragmentizer.h"
#include "interpreter/ExpressionFragmentizer.h"
#include "interpreter/HashTableSourceFragmentizer.h"
#include "interpreter/RuntimeExpressionFragmentizer.h"
#include "interpreter/RuntimeFunctionSubopFragmentizer.h"
#include "interpreter/RuntimeKeyExpressionFragmentizer.h"
#include "interpreter/KeyPackingFragmentizer.h"
#include "interpreter/TScanFragmentizer.h"

namespace inkfuse {

TypeDecorator::TypeDecorator(std::vector<IR::TypeArc> types_) : types(std::move(types_)) {
}

std::vector<IR::TypeArc> TypeDecorator::produce() {
   std::vector<IR::TypeArc> results = std::move(types);
   return results;
}

TypeDecorator& TypeDecorator::attachIntegers() {
   attachUnsignedIntegers();
   attachSignedIntegers();
   return *this;
}

TypeDecorator& TypeDecorator::attachUnsignedIntegers() {
   types.push_back(IR::SignedInt::build(8));
   types.push_back(IR::SignedInt::build(4));
   types.push_back(IR::SignedInt::build(2));
   types.push_back(IR::SignedInt::build(1));
   return *this;
}

TypeDecorator& TypeDecorator::attachSignedIntegers() {
   types.push_back(IR::UnsignedInt::build(8));
   types.push_back(IR::UnsignedInt::build(4));
   types.push_back(IR::UnsignedInt::build(2));
   types.push_back(IR::UnsignedInt::build(1));
   return *this;
}

TypeDecorator& TypeDecorator::attachFloatingPoints() {
   types.push_back(IR::Float::build(4));
   types.push_back(IR::Float::build(8));
   return *this;
}

TypeDecorator& TypeDecorator::attachNumeric() {
   attachIntegers();
   attachFloatingPoints();
   // We also count dates as numeric types. This is because a date internally 
   // is represented as a 4 byte signed integer (day offset to the epoch).
   types.push_back(IR::Date::build());
   return *this;
}

TypeDecorator& TypeDecorator::attachTypes() {
   attachNumeric();
   types.push_back(IR::Char::build());
   types.push_back(IR::Bool::build());
   return *this;
}

TypeDecorator& TypeDecorator::attachStringType()
{
   types.push_back(IR::String::build());
   return *this;
}

TypeDecorator& TypeDecorator::attachCharPtr() {
   types.push_back(IR::Pointer::build(IR::Char::build()));
   return *this;
}

TypeDecorator& TypeDecorator::attachPackedKeyTypes() {
   types.push_back(IR::ByteArray::build(0));
   types.push_back(IR::Pointer::build(IR::Char::build()));
   return *this;
}

const std::list<std::pair<std::string, Pipeline>>& Fragmentizer::getFragments() const {
   return pipes;
}

IR::ProgramArc FragmentGenerator::build() {
   // Set up the suboperator fragmentizers.
   std::vector<std::unique_ptr<Fragmentizer>> fragmentizers;
   fragmentizers.push_back(std::make_unique<AggregationFragmentizer>());
   fragmentizers.push_back(std::make_unique<TScanFragmetizer>());
   fragmentizers.push_back(std::make_unique<CopyFragmentizer>());
   fragmentizers.push_back(std::make_unique<ExpressionFragmentizer>());
   fragmentizers.push_back(std::make_unique<HashTableSourceFragmentizer>());
   fragmentizers.push_back(std::make_unique<RuntimeExpressionFragmentizer>());
   fragmentizers.push_back(std::make_unique<RuntimeKeyExpressionFragmentizer>());
   fragmentizers.push_back(std::make_unique<KeyPackingFragmentizer>());
   fragmentizers.push_back(std::make_unique<CountingSinkFragmentizer>());
   fragmentizers.push_back(std::make_unique<ColumnFilterFragmentizer>());
   fragmentizers.push_back(std::make_unique<RuntimeFunctionSubopFragmentizer>());

   // Create the IR program.
   auto program = std::make_shared<IR::Program>("fragments", false);
   // And generate the code for all fragments.
   for (auto& fragmentizer : fragmentizers) {
      for (const auto& [name, pipe] : fragmentizer->getFragments()) {
         // We now repipe the full pipeline. This is elegant, as it automatically takes care of generating
         // the right fuse-chunk input and output operators which are needed in the actual fragment.
         // This in turn means that sub-operators don't have to create fuse chunk sources and sinks themselves.
         auto repiped = pipe.repipeAll(0, pipe.getSubops().size());
         CompilationContext context(program, name, *repiped);
         context.compile();
      }
   }

   return program;
}

}
