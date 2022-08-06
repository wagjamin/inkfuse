#include "interpreter/KeyPackingFragmentizer.h"
#include "algebra/suboperators/row_layout/KeyPackerSubop.h"
#include "algebra/suboperators/row_layout/KeyUnpackerSubop.h"

namespace inkfuse {

namespace {

const std::vector<IR::TypeArc> types =
   TypeDecorator()
      .attachTypes()
      .produce();

}

KeyPackingFragmentizer::KeyPackingFragmentizer() {
   for (auto& type : types) {
      // Set up key packer suboperator fragments.
      auto& [name_packer, pipe_packer] = pipes.emplace_back();
      const auto& compound_key = generated_ius.emplace_back(IR::Pointer::build(IR::Char::build()), "compound_key");
      const auto& key_in = generated_ius.emplace_back(type, "iu_in");
      const auto& op_packer = pipe_packer.attachSuboperator(KeyPackerSubop::build(nullptr, key_in, compound_key));
      name_packer = op_packer.id();
   }
   for (auto& type: types) {
      // Set up key unpacker suboperator fragments.
      auto& [name_unpacker, pipe_unpacker] = pipes.emplace_back();
      const auto& key_out = generated_ius.emplace_back(type, "iu_out");
      const auto& compound_key = generated_ius.emplace_back(IR::Pointer::build(IR::Char::build()), "compound_key");
      const auto& op_unpacker = pipe_unpacker.attachSuboperator(KeyUnpackerSubop::build(nullptr, compound_key, key_out));
      name_unpacker = op_unpacker.id();
   }
}

}