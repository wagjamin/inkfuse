#include "algebra/Join.h"
#include "algebra/suboperators/ColumnFilter.h"
#include "algebra/suboperators/RuntimeFunctionSubop.h"
#include "algebra/suboperators/row_layout/KeyPackerSubop.h"
#include "algebra/suboperators/row_layout/KeyUnpackerSubop.h"
#include "algebra/suboperators/sources/ScratchPadIUProvider.h"
#include <algorithm>

namespace inkfuse {

namespace {

void allocHashTable(
   size_t key_size,
   size_t slot_size,
   size_t total_threads,
   TupleMaterializerState& mat,
   AtomicHashTableState<SimpleKeyComparator>& ht_state) {
   // 1. Allocate hash table.
   // Figure out how many rows were materialized.
   size_t total_rows = 0;
   for (const auto& buffer : mat.materializers) {
      total_rows += buffer.getNumTuples();
   }
   // We want at least 2x capacity and two slots to keep linear probing chains short.
   const size_t min_capacity = std::max(static_cast<size_t>(2), 2 * total_rows);
   // Total slots needed are next power of 2.
   const size_t total_slots = 1ull << (64 - __builtin_clzl(min_capacity - 1));
   // Allocate the hash table we will populate.
   ht_state.hash_table = std::make_unique<AtomicHashTable<SimpleKeyComparator>>(
      SimpleKeyComparator(key_size),
      slot_size,
      total_slots);
}

void materializedTupleToHashTable(
   size_t slot_size,
   size_t thread_id,
   TupleMaterializerState& mat,
   AtomicHashTableState<SimpleKeyComparator>& ht_state) {
   assert(ht_state.hash_table);
   assert(!mat.handles.empty());
   assert(mat.handles.size() == mat.materializers.size());
   const size_t batch_size = 256;
   std::vector<uint64_t> hashes(batch_size);
   for (auto& read_handle : mat.handles) {
      // Pick morsels from the read handle.
      while (const TupleMaterializer::MatChunk* chunk = read_handle->pullChunk()) {
         // Materialize all tuples from the chunk.
         // We traverse the materialized tuple in batches of 256 similar as a vectorized
         // engine would. For large hash tables this increases throughput significantly.
         const char* curr_tuple = reinterpret_cast<const char*>(chunk->data.get());
         while (curr_tuple < chunk->end_ptr) {
            size_t curr_batch_size = std::min(batch_size, (chunk->end_ptr - curr_tuple) / slot_size);
            const char* curr_tuple_hash_it = curr_tuple;
            for (size_t batch_idx = 0; batch_idx < curr_batch_size; ++batch_idx) {
               hashes[batch_idx] = ht_state.hash_table->compute_hash(curr_tuple_hash_it);
               curr_tuple_hash_it += slot_size;
            }
            for (size_t batch_idx = 0; batch_idx < curr_batch_size; ++batch_idx) {
               ht_state.hash_table->slot_prefetch(hashes[batch_idx]);
            }
            for (size_t batch_idx = 0; batch_idx < curr_batch_size; ++batch_idx) {
               ht_state.hash_table->insert<false>(curr_tuple, hashes[batch_idx]);
               curr_tuple += slot_size;
            }
            // Move to the next tuple.
         }
      }
   }
}
}

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
   full_row_left.reserve(keys_left.size() + payload_left.size());
   for (const IU* left_key : keys_left) {
      full_row_left.push_back(left_key);
   }
   for (const IU* left_payload : payload_left) {
      full_row_left.push_back(left_payload);
   }
   plan();
}

std::unique_ptr<Join> Join::build(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string op_name_, std::vector<const IU*> keys_left_, std::vector<const IU*> payload_left_, std::vector<const IU*> keys_right_, std::vector<const IU*> payload_right_, JoinType type_, bool is_pk_join_) {
   return std::make_unique<Join>(std::move(children_), std::move(op_name_), std::move(keys_left_), std::move(payload_left_), std::move(keys_right_), std::move(payload_right_), type_, is_pk_join_);
}

void Join::plan() {
   // We need to pack the entire left side into a dense row layout.
   // We then insert that into the TupleMaterializer. So both key and
   // payload columns get a pseudo IU.
   for (const IU* key : keys_left) {
      auto& iu = keys_left_out.emplace_back(key->type);
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

   scratch_pad_left.emplace(IR::ByteArray::build(key_size_left + payload_size_right));
   scratch_pad_right.emplace(IR::ByteArray::build(key_size_right + payload_size_right));
   filtered_build.emplace(IR::Pointer::build(IR::Char::build()));
   // The filtered probe column consists of Char* into the contiguous ByteArray column `filtered_build`.
   filtered_probe.emplace(IR::Pointer::build(IR::Char::build()));
   lookup_left.emplace(IR::Pointer::build(IR::Char::build()));
   lookup_right.emplace(IR::Pointer::build(IR::Char::build()));
   filter_pseudo_iu.emplace(IR::Void::build());

   // The probe hash is always a unit64_t.
   hash_right.emplace(IR::UnsignedInt::build(8));
   // Pseudo IU for making sure we prefetch before we probe.
   prefetch_pseudo.emplace(IR::Void::build());
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
   // 1. Pack the build side key + payload into a scratch pad IU
   // 2. Insert the scratch pad IU into a Tuple Materializer
   //
   // -> Runtime-Scheduled Hash Table Build Phase
   //
   // Probe pipeline:
   // 1. Pack both the probe key and the probe payload into a scratch pad IU
   // 2. Lookup the scratch pad IU
   // 3. Filter the rows whether the lookup returned a non-null pointer
   // 4. Unpack all the rows again into individual IUs

   auto& mat_state = dag.attachTupleMaterializers(0, key_size_left + payload_size_left);
   auto& ht_state = dag.attachAtomicHashTable<SimpleKeyComparator>(0, mat_state);
   {
      // Step 1: Construct the build pipeline.

      // 1.0: Decay build pipeline.
      children[0]->decay(dag);
      auto& build_pipe = dag.getCurrentPipeline();

      // 1.1 Materialize state. Maybe somewhat surprisingly we don't do an insert here at all.
      // Rather we follow the Hyper paper and have the following protocol:
      //   1. Every thread materializes all build-side tuples in row-layout into a thread-local TupleMaterializer.
      //   2. The pipeline finishes
      //   3. We allocate a perfectly sized hash table
      //   4. We kick off a multithreaded build phase in which the worker threads insert the materialized
      //      rows into the hash table.
      // Set up the materializer. It depends on all build-side columns, and generates a char* lookup.
      build_pipe.attachSuboperator(RuntimeFunctionSubop::materializeTuple(this, *lookup_left, full_row_left, &mat_state));

      // 1.2 Pack the entire left side into the allocated space from the TupleMaterializer.
      size_t build_row_offset = 0;
      for (const IU* col_row_left : full_row_left) {
         auto& packer = build_pipe.attachSuboperator(KeyPackerSubop::build(this, *col_row_left, *lookup_left, {}));
         // Attach the runtime parameter that represents the state offset.
         KeyPackingRuntimeParams param;
         param.offsetSet(IR::UI<2>::build(build_row_offset));
         reinterpret_cast<KeyPackerSubop&>(packer).attachRuntimeParams(std::move(param));
         // Update the offset by the size of the IU.
         build_row_offset += col_row_left->type->numBytes();
      }
   }

   // Intermediate runtime-scheduled step: build the actual hash table.
   dag.addRuntimeTask(
      PipelineDAG::RuntimeTask{
         .after_pipe = dag.getPipelines().size() - 1,
         .prepare_function = [&](ExecutionContext&, size_t total_threads) { allocHashTable(
                                                                               key_size_left,
                                                                               key_size_left + payload_size_left,
                                                                               total_threads,
                                                                               mat_state,
                                                                               ht_state); },
         .worker_function = [&](ExecutionContext&, size_t thread_id) { materializedTupleToHashTable(
                                                                          key_size_left + payload_size_left,
                                                                          thread_id,
                                                                          mat_state,
                                                                          ht_state); },
      });
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
      {
         {
            // Perform the actual lookup in a fully vectorized fashion.
            Pipeline::ROFScopeGuard rof_guard{probe_pipe};

            std::vector<const IU*> pseudo;
            for (const auto& pseudo_iu : right_pseudo_ius) {
               pseudo.push_back(&pseudo_iu);
            }

            // 2.2.1 Compute the hash.
            probe_pipe.attachSuboperator(RuntimeFunctionSubop::htHash<AtomicHashTable<SimpleKeyComparator>>(this, *hash_right, *scratch_pad_right, std::move(pseudo), &ht_state));

            // 2.2.2 Prefetch the slot.
            probe_pipe.attachSuboperator(RuntimeFunctionSubop::htPrefetch<AtomicHashTable<SimpleKeyComparator>>(this, &*prefetch_pseudo, *hash_right, &ht_state));

            // 2.2.3 Perfom the lookup.
            if (type == JoinType::LeftSemi) {
               // Lookup on a slot disables the slot, giving semi-join behaviour.
               probe_pipe.attachSuboperator(RuntimeFunctionSubop::htLookupWithHash<AtomicHashTable<SimpleKeyComparator>, true>(this, *lookup_right, *scratch_pad_right, *hash_right, &*prefetch_pseudo, &ht_state));
            } else {
               // Regular lookup that does not disable slots.
               probe_pipe.attachSuboperator(RuntimeFunctionSubop::htLookupWithHash<AtomicHashTable<SimpleKeyComparator>, false>(this, *lookup_right, *scratch_pad_right, *hash_right, &*prefetch_pseudo, &ht_state));
            }
         }

         // 2.3 Filter on probe matches.
         auto& filter_scope_subop = probe_pipe.attachSuboperator(ColumnFilterScope::build(this, *lookup_right, *filter_pseudo_iu));
         auto& filter_scope = reinterpret_cast<ColumnFilterScope&>(filter_scope_subop);
         // The filter on the build site filters "itself". This has some repercussions on the repiping
         // behaviour of the suboperator and needs to be passed explicitly.
         auto& filter_1 = probe_pipe.attachSuboperator(ColumnFilterLogic::build(this, *filter_pseudo_iu, *lookup_right, *filtered_build, /* filter_type= */ lookup_right->type, /* filters_itself= */ true));
         filter_scope.attachFilterLogicDependency(filter_1, *lookup_right);
         if (type != JoinType::LeftSemi) {
            // If we need to produce columns on the probe side, we also have to filter the probe result.
            // Note: the filtered ByteArray from the probe side becomes a Char* after filtering.
            auto& filter_2 = probe_pipe.attachSuboperator(ColumnFilterLogic::build(this, *filter_pseudo_iu, *scratch_pad_right, *filtered_probe, /* filter_type_= */ lookup_right->type));
            filter_scope.attachFilterLogicDependency(filter_2, *scratch_pad_right);
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
         // End vectorized Block.
      }
   }
}
}
