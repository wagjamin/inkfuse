#include "algebra/Join.h"
#include "algebra/suboperators/ColumnFilter.h"
#include "algebra/suboperators/RuntimeFunctionSubop.h"
#include "algebra/suboperators/row_layout/KeyPackerSubop.h"
#include "algebra/suboperators/row_layout/KeyUnpackerSubop.h"
#include "algebra/suboperators/sources/ScratchPadIUProvider.h"

namespace inkfuse {

Join::Join(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string op_name_, std::vector<const IU*> keys_left_, std::vector<const IU*> payload_left_, std::vector<const IU*> keys_right_, std::vector<const IU*> payload_right_, JoinType type_, bool is_pk_join_)
   : RelAlgOp(std::move(children_), std::move(op_name_)),
     type(type_),
     is_pk_join(is_pk_join_),
     keys_left(std::move(keys_left_)),
     payload_left(std::move(payload_left_)),
     keys_right(std::move(keys_right_)),
     payload_right(std::move(payload_right_)) {
   if (children.size() != 2) {
      throw std::runtime_error("Join needs to have two children");
   }
   plan();
}

std::unique_ptr<Join> Join::build(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string op_name_, std::vector<const IU*> keys_left_, std::vector<const IU*> payload_left_, std::vector<const IU*> keys_right_, std::vector<const IU*> payload_right_, JoinType type_, bool is_pk_join_) {
   return std::make_unique<Join>(std::move(children_), std::move(op_name_), std::move(keys_left_), std::move(payload_left_), std::move(keys_right_), std::move(payload_right_), type_, is_pk_join_);
}

void Join::plan() {
   // Set up the left side output IUs.
   for (const IU* key : keys_left) {
      auto& iu = keys_left_out.emplace_back(key->type);
      if (keys_left.size() > 1) {
         left_pseudo_ius.emplace_back(IR::Void::build());
      }
      output_ius.push_back(&iu);
      key_size_left += iu.type->numBytes();
   }
   for (const IU* payload : payload_left) {
      auto& iu = payload_left_out.emplace_back(payload->type);
      output_ius.push_back(&iu);
      payload_size_left += iu.type->numBytes();
   }

   if (type == JoinType::LeftSemi) {
      // A left semi join should not have any payload on the probe side.
      assert(payload_right.empty());
   }

   // Set up the right side output IUs.
   for (const IU* key : keys_right) {
      if (type != JoinType::LeftSemi) {
         auto& iu = keys_right_out.emplace_back(key->type);
         output_ius.push_back(&iu);
      }
      right_pseudo_ius.emplace_back(IR::Void::build());
      key_size_right += key->type->numBytes();
   }
   for (const IU* payload : payload_right) {
      auto& iu = payload_right_out.emplace_back(payload->type);
      // We need pseudo IUs for the payload of the probe side. The return IU of the probe
      // gets used for re-unpacking the joined row later. When we touch that return IU we
      // need to make sure the entire key is present already.
      right_pseudo_ius.emplace_back(IR::Void::build());
      output_ius.push_back(&iu);
      payload_size_right += iu.type->numBytes();
   }

   assert(key_size_left == key_size_right);

   scratch_pad_left.emplace(IR::ByteArray::build(key_size_left));
   scratch_pad_right.emplace(IR::ByteArray::build(key_size_right + payload_size_right));
   filtered_build.emplace(IR::Pointer::build(IR::Char::build()));
   // The filtered probe column consists of Char* into the contiguous ByteArray column `filtered_build`.
   filtered_probe.emplace(IR::Pointer::build(IR::Char::build()));
   lookup_left.emplace(IR::Pointer::build(IR::Char::build()));
   lookup_right.emplace(IR::Pointer::build(IR::Char::build()));
   filter_pseudo_iu.emplace(IR::Void::build());
}

void Join::decay(inkfuse::PipelineDAG& dag) const {
   if (is_pk_join) {
      decayPkJoin(dag);
   } else {
      // TODO(benjamin) implement
      throw std::runtime_error("Only supporting PK joins so far");
   }
}

void Join::decayPkJoin(inkfuse::PipelineDAG& dag) const {
   // Decay a primary key join into a DAG of suboperators. This proceeds as-follows:
   //
   // Build pipeline:
   // 1. Pack the join key into a scratch pad IU
   // 2. Insert the scratch pad IU into a hash table
   // 3. Pack the remaining columns of the build payload
   //
   // Probe pipeline:
   // 1. Pack both the probe key and the probe payload into a scratch pad IU
   // 2. Lookup the scratch pad IU
   // 3. Filter the rows whether the lookup returned a non-null pointer
   // 4. Unpack all the rows again into individual IUs

   // TODO fix discard
   HashTableSimpleKeyState& ht = dag.attachHashTableSimpleKey(0, key_size_left, payload_size_left);
   {
      // Step 1: Construct the build pipeline.

      // 1.0: Decay build pipeline.
      children[0]->decay(dag);
      auto& build_pipe = dag.getCurrentPipeline();

      // 1.1 Pack the join key into a scratch pad IU.
      const IU* key_iu = keys_left.front();
      if (keys_left.size() > 1) {
         key_iu = &(*scratch_pad_left);
         // We only need to pack if we can't use the single key directly.
         build_pipe.attachSuboperator(ScratchPadIUProvider::build(this, *scratch_pad_left));
         size_t build_key_offset = 0;
         auto build_pseudo = left_pseudo_ius.begin();
         for (const IU* key_left : keys_left) {
            auto& packer = build_pipe.attachSuboperator(KeyPackerSubop::build(this, *key_left, *scratch_pad_left, {&(*build_pseudo)}));
            // Attach the runtime parameter that represents the state offset.
            KeyPackingRuntimeParams param;
            param.offsetSet(IR::UI<2>::build(build_key_offset));
            reinterpret_cast<KeyPackerSubop&>(packer).attachRuntimeParams(std::move(param));
            // Update the key offset by the size of the IU.
            build_key_offset += key_left->type->numBytes();
            build_pseudo++;
         }
      }

      // 1.2 Insert into the hash table.
      std::vector<const IU*> pseudo;
      for (const auto& pseudo_iu : left_pseudo_ius) {
         pseudo.push_back(&pseudo_iu);
      }
      // We know there are no duplicate keys. We might think we can insert directly, without duplicate checking.
      // However, this is not possible since there might be morsel restarts. We need `htLookupOrInsert`.
      if (payload_left.empty()) {
         // We do not care about the result pointer as we don't need to do packing.
         build_pipe.attachSuboperator(RuntimeFunctionSubop::htLookupOrInsert<HashTableSimpleKey>(this, nullptr, *key_iu, std::move(pseudo), &ht));
      } else {
         // We need the result pointer for payload packing.
         build_pipe.attachSuboperator(RuntimeFunctionSubop::htLookupOrInsert<HashTableSimpleKey>(this, &(*lookup_left), *key_iu, std::move(pseudo), &ht));
      }

      // 1.3 Pack the payload.
      size_t build_payload_offset = key_size_left;
      for (const IU* payload : payload_left) {
         auto& packer = build_pipe.attachSuboperator(KeyPackerSubop::build(this, *payload, *lookup_left, {}));
         // Attach the runtime parameter that represents the state offset.
         KeyPackingRuntimeParams param;
         param.offsetSet(IR::UI<2>::build(build_payload_offset));
         reinterpret_cast<KeyPackerSubop&>(packer).attachRuntimeParams(std::move(param));
         // Update the key offset by the size of the IU.
         build_payload_offset += payload->type->numBytes();
      }
   }

   {
      // Step 2: Construct the probe pipeline.

      // 2.0 : Decay probe pipeline.
      children[1]->decay(dag);
      auto& probe_pipe = dag.getCurrentPipeline();

      // 2.1 Pack the probe key and the probe payload.
      probe_pipe.attachSuboperator(ScratchPadIUProvider::build(this, *scratch_pad_right));
      size_t probe_offset = 0;
      auto probe_pseudo = right_pseudo_ius.begin();
      // Pack keys.
      for (const IU* key_right : keys_right) {
         auto& packer = probe_pipe.attachSuboperator(KeyPackerSubop::build(this, *key_right, *scratch_pad_right, {&(*probe_pseudo)}));
         // Attach the runtime parameter that represents the state offset.
         KeyPackingRuntimeParams param;
         param.offsetSet(IR::UI<2>::build(probe_offset));
         reinterpret_cast<KeyPackerSubop&>(packer).attachRuntimeParams(std::move(param));
         // Update the key offset by the size of the IU.
         probe_offset += key_right->type->numBytes();
         probe_pseudo++;
      }
      // Pack payload.
      for (const IU* payload_r : payload_right) {
         auto& packer = probe_pipe.attachSuboperator(KeyPackerSubop::build(this, *payload_r, *scratch_pad_right, {&(*probe_pseudo)}));
         // Attach the runtime parameter that represents the state offset.
         KeyPackingRuntimeParams param;
         param.offsetSet(IR::UI<2>::build(probe_offset));
         reinterpret_cast<KeyPackerSubop&>(packer).attachRuntimeParams(std::move(param));
         // Update the key offset by the size of the IU.
         probe_offset += payload_r->type->numBytes();
         probe_pseudo++;
      }

      // 2.2 Probe.
      std::vector<const IU*> pseudo;
      for (const auto& pseudo_iu : right_pseudo_ius) {
         pseudo.push_back(&pseudo_iu);
      }

      if (type == JoinType::LeftSemi) {
         // Lookup on a slot disables the slot, giving semi-join behaviour.
         probe_pipe.attachSuboperator(RuntimeFunctionSubop::htLookupDisable(this, *lookup_right, *scratch_pad_right, std::move(pseudo), &ht));
      } else {
         // Regular lookup that does not disable slots.
         probe_pipe.attachSuboperator(RuntimeFunctionSubop::htLookup<HashTableSimpleKey>(this, *lookup_right, *scratch_pad_right, std::move(pseudo), &ht));
      }

      // 2.3 Filter on probe matches.
      probe_pipe.attachSuboperator(ColumnFilterScope::build(this, *lookup_right, *filter_pseudo_iu));
      // The filter on the build site filters "itself". This has some repercussions on the repiping
      // behaviour of the suboperator and needs to be passed explicitly.
      probe_pipe.attachSuboperator(ColumnFilterLogic::build(this, *filter_pseudo_iu, *lookup_right, *filtered_build, /* filter_type= */ lookup_right->type, /* filters_itself= */ true));
      if (type != JoinType::LeftSemi) {
         // If we need to produce columns on the probe side, we also have to filter the probe result.
         // Note: the filtered ByteArray from the probe side becomes a Char* after filtering.
         probe_pipe.attachSuboperator(ColumnFilterLogic::build(this, *filter_pseudo_iu, *scratch_pad_right, *filtered_probe, /* filter_type_= */ lookup_right->type));
      }

      // 2.4 Unpack everything.
      // 2.4.1 Unpack Build Side IUs
      size_t build_unpack_offset = 0;
      for (const auto& iu : keys_left_out) {
         auto& unpacker = probe_pipe.attachSuboperator(KeyUnpackerSubop::build(this, *filtered_build, iu));
         KeyPackingRuntimeParams param;
         param.offsetSet(IR::UI<2>::build(build_unpack_offset));
         reinterpret_cast<KeyUnpackerSubop&>(unpacker).attachRuntimeParams(std::move(param));
         build_unpack_offset += iu.type->numBytes();
      }
      for (const auto& iu : payload_left_out) {
         auto& unpacker = probe_pipe.attachSuboperator(KeyUnpackerSubop::build(this, *filtered_build, iu));
         KeyPackingRuntimeParams param;
         param.offsetSet(IR::UI<2>::build(build_unpack_offset));
         reinterpret_cast<KeyUnpackerSubop&>(unpacker).attachRuntimeParams(std::move(param));
         build_unpack_offset += iu.type->numBytes();
      }
      // 2.4.1 Unpack Probe Side IUs. Not needed for semi joins.
      if (type != JoinType::LeftSemi) {
         size_t probe_unpack_offset = 0;
         for (const auto& iu : keys_right_out) {
            auto& unpacker = probe_pipe.attachSuboperator(KeyUnpackerSubop::build(this, *filtered_probe, iu));
            KeyPackingRuntimeParams param;
            param.offsetSet(IR::UI<2>::build(probe_unpack_offset));
            reinterpret_cast<KeyUnpackerSubop&>(unpacker).attachRuntimeParams(std::move(param));
            probe_unpack_offset += iu.type->numBytes();
         }
         for (const auto& iu : payload_right_out) {
            auto& unpacker = probe_pipe.attachSuboperator(KeyUnpackerSubop::build(this, *filtered_probe, iu));
            KeyPackingRuntimeParams param;
            param.offsetSet(IR::UI<2>::build(probe_unpack_offset));
            reinterpret_cast<KeyUnpackerSubop&>(unpacker).attachRuntimeParams(std::move(param));
            probe_unpack_offset += iu.type->numBytes();
         }
      }
   }
}

}
