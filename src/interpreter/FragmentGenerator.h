#ifndef INKFUSE_FRAGMENTGENERATOR_H
#define INKFUSE_FRAGMENTGENERATOR_H

#include "algebra/Pipeline.h"
#include "codegen/Type.h"
#include "codegen/IR.h"
#include <vector>
#include <list>

/// The FragmentGenerator is responsible for generating vectorized, pre-compiled fragments.
/// During the compilation of the initial binary, these are generated and put into a shared object file.
/// This object file can then be dynamically opened on inkfuse startup. It is later used by the FragmentCache
/// to quickly provide the QueryExecutor access to the vectorized fragments during interpretation.
namespace inkfuse {

/// Builds a vector of types in a nice way.
struct TypeDecorator {
   TypeDecorator(std::vector<IR::TypeArc> types_ = {});

   /// Get the vector of all types.
   std::vector<IR::TypeArc> produce();

   /// Add all integer types.
   TypeDecorator& attachIntegers();
   /// Add all unsigned integer types.
   TypeDecorator& attachUnsignedIntegers();
   /// Add all signed integer types.
   TypeDecorator& attachSignedIntegers();

   private:
   std::vector<IR::TypeArc> types;
};

/// Fragmentizer can generate a set of vectorized fragments.
struct Fragmentizer {

   virtual ~Fragmentizer() = default;

   /// Get all fragments of this specific fragmentizer.
   /// A fragment is identified by a unique name and a pipeline of sub-operators.
   virtual const std::list<std::pair<std::string, Pipeline>>& getFragments() const = 0;

};

namespace FragmentGenerator {

/// Build the full set of fragments.
IR::ProgramArc build();

}

} // namespace inkfuse

#endif //INKFUSE_FRAGMENTGENERATOR_H
