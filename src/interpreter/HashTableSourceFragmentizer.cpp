#include "interpreter/HashTableSourceFragmentizer.h"
#include "algebra/suboperators/sources/HashTableSource.h"

namespace inkfuse {

HashTableSourceFragmentizer::HashTableSourceFragmentizer() {
   {
      auto& [name, pipe] = pipes.emplace_back();
      auto& target_iu = generated_ius.emplace_back(IR::Pointer::build(IR::Char::build()), "");
      auto& op = pipe.attachSuboperator(SimpleHashTableSource::build(nullptr, target_iu, nullptr));
      name = op.id();
   }
   {
      auto& [name, pipe] = pipes.emplace_back();
      auto& target_iu = generated_ius.emplace_back(IR::Pointer::build(IR::Char::build()), "");
      auto& op = pipe.attachSuboperator(ComplexHashTableSource::build(nullptr, target_iu, nullptr));
      name = op.id();
   }
   {
      auto& [name, pipe] = pipes.emplace_back();
      auto& target_iu = generated_ius.emplace_back(IR::Pointer::build(IR::Char::build()), "");
      auto& op = pipe.attachSuboperator(DirectLookupHashTableSource::build(nullptr, target_iu, nullptr));
      name = op.id();
   }
   {
      auto& [name, pipe] = pipes.emplace_back();
      auto& target_iu = generated_ius.emplace_back(IR::Pointer::build(IR::Char::build()), "");
      auto& marker_iu = generated_ius.emplace_back(IR::ByteArray::build(5), "");
      auto& op = pipe.attachSuboperator(AtomicHashTableSource::buildForOuterJoin(nullptr, target_iu, marker_iu, nullptr));
      name = op.id();
   }
}
    
} // namespace inkfuse
