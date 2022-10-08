#include "interpreter/HashTableSourceFragmentizer.h"
#include "algebra/suboperators/sources/HashTableSource.h"

namespace inkfuse {

HashTableSourceFragmentizer::HashTableSourceFragmentizer() {
      auto& [name, pipe] = pipes.emplace_back();
      auto& target_iu = generated_ius.emplace_back(IR::Pointer::build(IR::Char::build()), "");
      auto& op = pipe.attachSuboperator(HashTableSource::build(nullptr, target_iu, nullptr));
      name = op.id();
}
    
} // namespace inkfuse
