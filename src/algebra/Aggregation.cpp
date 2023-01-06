#include "algebra/Aggregation.h"
#include "algebra/AggFunctionRegisty.h"
#include "algebra/Pipeline.h"
#include "algebra/suboperators/RuntimeFunctionSubop.h"
#include "algebra/suboperators/aggregation/AggReaderSubop.h"
#include "algebra/suboperators/aggregation/AggregatorSubop.h"
#include "algebra/suboperators/expressions/ExpressionSubop.h"
#include "algebra/suboperators/row_layout/KeyUnpackerSubop.h"
#include "algebra/suboperators/sources/HashTableSource.h"
#include "algebra/suboperators/sources/ScratchPadIUProvider.h"
#include <map>

namespace inkfuse {

Aggregation::Aggregation(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string op_name_, std::vector<const IU*> group_by_, std::vector<AggregateFunctions::Description> aggregates_)
   : RelAlgOp(std::move(children_), std::move(op_name_)), group_by(std::move(group_by_)),
     agg_pointer_result(IR::Pointer::build(IR::Char::build())), ht_scan_result(IR::Pointer::build(IR::Char::build())) {
   plan(std::move(aggregates_));
}

std::unique_ptr<Aggregation> Aggregation::build(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string op_name_, std::vector<const IU*> group_by_, std::vector<AggregateFunctions::Description> aggregates_)
{
   return std::make_unique<Aggregation>(std::move(children_), std::move(op_name_), std::move(group_by_), std::move(aggregates_));
}

void Aggregation::plan(std::vector<AggregateFunctions::Description> description) {
   // We pack the aggregation keys first.
   for (const IU* key: group_by) {
      key_size += key->type->numBytes();
      const auto& out = out_key_ius.emplace_back(key->type);
      output_ius.push_back(&out);
      if (group_by.size() != 1) {
         // If there is more than one key, we will need a pseudo IU later
         // to indicate that key packing needs to happen before the hash table insert.
         pseudo_ius.emplace_back(IR::Void::build());
      }
   }
   // We know the key size - so we can now create the properly sized byte array.
   packed_ht_key.emplace(IR::ByteArray::build(key_size));

   // Basic planning of the aggregate key. There are some easy performance improvements:
   // - Compute granule layout ordered by IU
   // - Proper aligned state
   // The granules on a given IU that were computed already mapping to the offset in the 'granules' vector.

   // Mapping from the granule ID to its payload state offset within the payload.
   std::map<std::pair<const IU*, std::string>, size_t> computed_granules;
   for (auto& func : description) {
      // Get the suboperators that make up this aggregate function.
      auto ops = AggregateFunctions::lookupSubops(func);
      std::vector<size_t> granule_offsets;
      granule_offsets.reserve(ops.agg_reader->requiredGranules());
      for (auto& granule : ops.granules) {
         std::pair<const IU*, std::string> granule_id = {&func.agg_iu, granule->id()};
         if (computed_granules.count(granule_id) == 0) {
            // First time we see this granule, add it to the total set of granules.
            size_t granule_state_size = granule->getStateSize();
            granules.emplace_back(&func.agg_iu, std::move(granule));
            // Offset within the packed payload is the current offset.
            computed_granules.emplace(granule_id, payload_size);
            granule_offsets.push_back(payload_size);
            // Increase payload size by the state size of the fresh granule.
            payload_size += granule_state_size;
         } else {
            // We already computed this granule, reuse it.
            auto offset = computed_granules.at(granule_id);
            granule_offsets.push_back(offset);
         }
      }
      // Construct plan for this aggregation.
      compute.push_back(PlannedAggCompute{
         .compute = std::move(ops.agg_reader),
         .granule_offsets = std::move(granule_offsets),
      });
      // And set up the output iu.
      assert(ops.result_type.get());
      auto& out_iu = out_aggregate_ius.emplace_back(ops.result_type);
      output_ius.push_back(&out_iu);
   }
}

void Aggregation::decay(PipelineDAG& dag) const {
   for (const auto& child : children) {
      child->decay(dag);
   }

   // Decay the full aggregation operation into a DAG of suboperators. This proceeds as-follows:
   // Original pipeline:
   // 1. Pack the aggregation key (if it needs to be packed)
   // 2. Lookup or insert the key in the aggregation hash table
   // 3. Update the aggregate state.
   // New Pipeline:
   // 4. Attach readers on a new pipeline.

   // Set up the backing aggregation hash table. We can drop it after the
   // pipeline which reads from the aggregation is done.
   auto& hash_table = dag.attachHashTableSimpleKey(dag.getPipelines().size(), key_size, payload_size);

   auto& curr_pipe = dag.getCurrentPipeline();
   // Step 1: Pack the aggregation key (if it needs to be packed)
   const IU* packed_key_iu;
   if (group_by.size() == 1) {
      // There is only a single key, we can use that directly.
      packed_key_iu = group_by[0];
   } else {
      // We have to pack the aggregation key. First provide a scratch pad IU.
      curr_pipe.attachSuboperator(ScratchPadIUProvider::build(this, *packed_ht_key));

      // Now pack the key into the provided scratch pad IU.
      size_t key_offset = 0;
      auto pseudo = pseudo_ius.begin();
      for (const auto& key: group_by) {
         auto& packer = curr_pipe.attachSuboperator(KeyPackerSubop::build(this, *key, *packed_ht_key, {&(*pseudo)}));
         // Attach the runtime parameter that represents the state offset.
         KeyPackingRuntimeParams param;
         param.offsetSet(IR::UI<2>::build(key_offset));
         reinterpret_cast<KeyPackerSubop&>(packer).attachRuntimeParams(std::move(param));
         // Update the key offset by the size of the IU.
         key_offset += key->type->numBytes();
         pseudo++;
      }
      packed_key_iu = &packed_ht_key.value();
   }

   // Step 2: Lookup or insert the key in the aggregation hash table
   std::vector<const IU*> pseudo;
   for (const auto& pseudo_iu: pseudo_ius) {
      pseudo.push_back(&pseudo_iu);
   }

   if (key_size) {
      curr_pipe.attachSuboperator(RuntimeFunctionSubop::htLookupOrInsert(this, &agg_pointer_result, *packed_key_iu, std::move(pseudo), &hash_table));
   } else {
      // The key size is zero - so we just aggregate a single group.
      // We use an optimized code path for this. We need to htNoKeyLookup to reference an
      // input IU as a dependency. This is
      curr_pipe.attachSuboperator(RuntimeFunctionSubop::htNoKeyLookup(this, agg_pointer_result, *granules[0].first, &hash_table));
   }

   // Step 3: Update the aggregation state.
   // The current offset into the serialized aggregate state. The aggregate state starts after the serialized key.
   size_t curr_offset = key_size;
   for (const auto& [agg_iu, agg_state] : granules) {
      // Create the aggregator which will compute the granule.
      auto& aggregator = reinterpret_cast<AggregatorSubop&>(curr_pipe.attachSuboperator(AggregatorSubop::build(this, *agg_state, agg_pointer_result, *agg_iu)));
      // Attach the runtime parameter that represents the state offset.
      KeyPackingRuntimeParams param;
      param.offsetSet(IR::UI<2>::build(curr_offset));
      aggregator.attachRuntimeParams(std::move(param));
      // Update the offset - the next aggregation state granule lives next to this one.
      curr_offset += agg_state->getStateSize();
   }

   // Step 4: Attach readers on a new pipeline.
   auto& read_pipe = dag.buildNewPipeline();
   // First, build a reader on the aggregate hash table returning pointers to the elements.
   read_pipe.attachSuboperator(HashTableSource::build(this, ht_scan_result, &hash_table));

   // Produce the readers for the materialized keys.
   size_t key_offset = 0;
   for (const auto& out_key_iu : out_key_ius) {
      auto& unpacker = read_pipe.attachSuboperator(KeyUnpackerSubop::build(this, ht_scan_result, out_key_iu));
      // Attach the runtime parameter that represents the state offset.
      KeyPackingRuntimeParams param;
      param.offsetSet(IR::UI<2>::build(key_offset));
      reinterpret_cast<KeyUnpackerSubop&>(unpacker).attachRuntimeParams(std::move(param));
      // Update the key offset by the size of the IU.
      key_offset += out_key_iu.type->numBytes();
   }

   // Produce the actual operators that computes the aggregate functions.
   auto out_compute = compute.cbegin();
   for (const auto& out_agg_iu : out_aggregate_ius) {
      auto& reader = reinterpret_cast<AggReaderSubop&>(read_pipe.attachSuboperator(AggReaderSubop::build(this, ht_scan_result, out_agg_iu, *out_compute->compute)));
      // Attach the runtime parameter that represents the state offset.
      const auto& g_offsets = out_compute->granule_offsets;
         KeyPackingRuntimeParamsTwo param;
      param.offset_1Set(IR::UI<2>::build(key_size + g_offsets[0]));
      if (out_compute->compute->requiredGranules() == 2) {
         assert(out_compute->granule_offsets.size() == 2);
         param.offset_2Set(IR::UI<2>::build(key_size + g_offsets[1]));
      }
      reader.attachRuntimeParams(std::move(param));
      out_compute++;
   }
}

}
