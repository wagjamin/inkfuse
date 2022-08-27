#include "interpreter/KeyPackingFragmentizer.h"
#include "algebra/suboperators/row_layout/KeyPackerSubop.h"
#include "algebra/suboperators/row_layout/KeyUnpackerSubop.h"

namespace inkfuse {

namespace {

const std::vector<IR::TypeArc> types =
   TypeDecorator()
      .attachTypes()
      .produce();

// We can pack/unpack into raw char* or explicit byte arrays. Size 1 is a placeholder, does not affect the generated code.
const std::vector<IR::TypeArc> source_types = {IR::Pointer::build(IR::Char::build()), IR::ByteArray::build(1)};

}

KeyPackingFragmentizer::KeyPackingFragmentizer() {
   for (auto& type : types) {
      for (auto& source_type: source_types) {
         // Set up key packer suboperator fragments.
         auto& [name_packer, pipe_packer] = pipes.emplace_back();
         const auto& compound_key = generated_ius.emplace_back(source_type, "compound_key");
         const auto& key_in = generated_ius.emplace_back(type, "iu_in");
         const auto& op_packer = pipe_packer.attachSuboperator(KeyPackerSubop::build(nullptr, key_in, compound_key));
         name_packer = op_packer.id();
      }
   }
   for (auto& type: types) {
      for (auto& source_type: source_types) {
         // Set up key unpacker suboperator fragments.
         auto& [name_unpacker, pipe_unpacker] = pipes.emplace_back();
         const auto& key_out = generated_ius.emplace_back(type, "iu_out");
         const auto& compound_key = generated_ius.emplace_back(source_type, "compound_key");
         const auto& op_unpacker = pipe_unpacker.attachSuboperator(KeyUnpackerSubop::build(nullptr, compound_key, key_out));
         name_unpacker = op_unpacker.id();
      }
   }
}

}