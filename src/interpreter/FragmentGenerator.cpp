#include "algebra/CompilationContext.h"
#include "interpreter/FragmentGenerator.h"
#include "interpreter/TScanFragmentizer.h"
#include "interpreter/CopyFragmentizer.h"

namespace inkfuse {

TypeDecorator::TypeDecorator(std::vector<IR::TypeArc> types_): types(std::move(types_))
{
}

std::vector<IR::TypeArc> TypeDecorator::produce() {
   std::vector<IR::TypeArc> results = std::move(types);
   return results;
}

TypeDecorator& TypeDecorator::attachIntegers()
{
   attachUnsignedIntegers();
   return attachSignedIntegers();
}

TypeDecorator& TypeDecorator::attachUnsignedIntegers()
{
   types.push_back(IR::SignedInt::build(8));
   types.push_back(IR::SignedInt::build(4));
   types.push_back(IR::SignedInt::build(2));
   types.push_back(IR::SignedInt::build(1));
   return *this;
}

TypeDecorator& TypeDecorator::attachSignedIntegers()
{
   types.push_back(IR::UnsignedInt::build(8));
   types.push_back(IR::UnsignedInt::build(4));
   types.push_back(IR::UnsignedInt::build(2));
   types.push_back(IR::UnsignedInt::build(1));
   return *this;
}

IR::ProgramArc FragmentGenerator::build()
{
   // Set up the suboperator fragmentizers.
   std::vector<std::unique_ptr<Fragmentizer>> fragmentizers;
   fragmentizers.push_back(std::make_unique<TScanFragmetizer>());
   fragmentizers.push_back(std::make_unique<CopyFragmentizer>());

   // Create the IR program.
   auto program = std::make_shared<IR::Program>("fragments", false);
   // And generate the code for all fragments.
   for (auto& fragmentizer: fragmentizers) {
      for (const auto& [name, pipe]: fragmentizer->getFragments()) {
         // We now repipe the full pipeline. This is elegant, as it automatically takes care of generating
         // the right fuse-chunk input and output operators which are needed in the actual fragment.
         // This in turn means that sub-operators don't have to create fuse chunk sources and sinks themselves.
         auto repiped = pipe.repipe(0, pipe.getSubops().size(), true);
         CompilationContext context(program, name, *repiped);
         context.compile();
      }
   }

   return program;
}

}
