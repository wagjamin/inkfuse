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
#include <functional>
#include <map>
#include <numeric>
#include <set>

namespace inkfuse {

Aggregation::Aggregation(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string op_name_, std::vector<const IU*> group_by_, std::vector<AggregateFunctions::Description> aggregates_)
   : RelAlgOp(std::move(children_), std::move(op_name_)), group_by(std::move(group_by_)),
     agg_pointer_result(IR::Pointer::build(IR::Char::build())), ht_scan_result(IR::Pointer::build(IR::Char::build())) {
   plan(std::move(aggregates_));
}

std::unique_ptr<Aggregation> Aggregation::build(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string op_name_, std::vector<const IU*> group_by_, std::vector<AggregateFunctions::Description> aggregates_) {
   return std::make_unique<Aggregation>(std::move(children_), std::move(op_name_), std::move(group_by_), std::move(aggregates_));
}

void Aggregation::plan(std::vector<AggregateFunctions::Description> description) {
   // Compute the hash table required by this aggregation?
   if (group_by.size() == 1 && dynamic_cast<IR::String*>(group_by[0]->type.get())) {
      requires_complex_ht = true;
   }
   // We pack the aggregation keys first.
   for (const IU* key : group_by) {
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

   // Plan the aggregate key. We perform the following optimizations when optimizing aggregate state:
   // - An aggregate function is divided into a 'compute' and 'extract' step.
   //   A query such as SELECT sum(x), avg(x) FROM T can share the sum state across both aggregate functions.
   //   We call this "granule", and turn the aggregate into a minimal set of aggregate granules.
   // - We then properly align the aggregate state. Even though x86 supports it, there should be no unaligned
   //   writes when doing aggregate state updates.
   // - For state with the same alignment requirement, we try to put aggregate updates on the same IU next
   //   to each other. This hopefully minimizes cache misses as it gets more likely that successive updates
   //   happen on the same cache line.

   // The granules we need to compute. Outlined as (granule_size, iu, compute_id).
   // The order of the set gives us a good aggregate state setup.
   // Note that the lexicographic ordering on (granule_size, iu) allows for both efficient aligned
   // access, as well as putting the same IU updates next to each other.
   using GranuleDescription = std::tuple<size_t, const IU*, std::string>;
   std::set<GranuleDescription, std::greater<>> to_compute;
   // The actual aggregate functions we have to compute.
   std::vector<std::pair<const IU*, AggregateFunctions::RegistryEntry>> functions;
   functions.reserve(description.size());

   // Extract all the granules we need to compute.
   for (const auto& func : description) {
      // Get the suboperators that make up this aggregate function.
      const auto& [iu, entry] = functions.emplace_back(&func.agg_iu, AggregateFunctions::lookupSubops(func));
      // Add the granules we have to compute.
      for (const auto& granule : entry.granules) {
         to_compute.insert({granule->getStateSize(), iu, granule->id()});
      }
   }
   granules.resize(to_compute.size());

   assert(!to_compute.empty());
   // The `granules` set did all the heavy lifting for us - now we just have to extract them
   // and re-attach the correct offsets to the aggregate functions.
   const size_t largest_state = std::get<0>(*to_compute.rbegin());
   // Find the starting offset for serializing the payload state.
   // This is the first aligned offset after the key.
   payload_offset = 0;
   while (payload_offset < key_size) {
      payload_offset += largest_state;
   }
   // We measure the offset _after_ the key, so subtract key size again.
   payload_offset -= key_size;
   auto offset_accumulator = [](size_t sum, const GranuleDescription& desc) {
      return sum + std::get<0>(desc);
   };
   payload_size = std::accumulate(to_compute.begin(), to_compute.end(), payload_offset, offset_accumulator);

   // Now plan each aggregate function.
   for (auto& [iu, entry] : functions) {
      std::vector<size_t> granule_offsets;
      granule_offsets.reserve(entry.granules.size());
      for (auto& granule : entry.granules) {
         auto granule_it = to_compute.find({granule->getStateSize(), iu, granule->id()});
         auto idx = std::distance(to_compute.begin(), granule_it);
         // Compute the prefix sum of all previous .
         const auto granule_offset = std::accumulate(to_compute.begin(), granule_it, payload_offset, offset_accumulator);
         granule_offsets.push_back(granule_offset);
         // Last writer wins - all granules are the same, so it doesn't matter which `AggStatePtr` we use.
         granules[idx] = {iu, std::move(granule)};
      }
      // Construct plan for this aggregate function.
      compute.push_back(PlannedAggCompute{
         .compute = std::move(entry.agg_reader),
         .granule_offsets = std::move(granule_offsets),
      });
      // And set up the output iu.
      assert(entry.result_type.get());
      auto& out_iu = out_aggregate_ius.emplace_back(entry.result_type);
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
   DefferredStateInitializer* hash_table;
   if (requires_complex_ht) {
      hash_table = &dag.attachHashTableComplexKey(dag.getPipelines().size(), 1, payload_size);
   } else if (key_size == 2) {
      hash_table = &dag.attachHashTableDirectLookup(dag.getPipelines().size(), payload_size);
   } else {
      hash_table = &dag.attachHashTableSimpleKey(dag.getPipelines().size(), key_size, payload_size);
   }

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
      for (const auto& key : group_by) {
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
   for (const auto& pseudo_iu : pseudo_ius) {
      pseudo.push_back(&pseudo_iu);
   }

   // Dispatch the correct lookup function.
   if (key_size && requires_complex_ht) {
      curr_pipe.attachSuboperator(RuntimeFunctionSubop::htLookupOrInsert<HashTableComplexKey>(this, &agg_pointer_result, *packed_key_iu, std::move(pseudo), hash_table));
   } else if (key_size == 2) {
      curr_pipe.attachSuboperator(RuntimeFunctionSubop::htLookupOrInsert<HashTableDirectLookup>(this, &agg_pointer_result, *packed_key_iu, std::move(pseudo), hash_table));
   } else if (key_size != 0) {
      curr_pipe.attachSuboperator(RuntimeFunctionSubop::htLookupOrInsert<HashTableSimpleKey>(this, &agg_pointer_result, *packed_key_iu, std::move(pseudo), hash_table));
   } else {
      // The key size is zero - so we just aggregate a single group.
      // We use an optimized code path for this. We need to htNoKeyLookup to reference an
      // input IU as a dependency. This is
      curr_pipe.attachSuboperator(RuntimeFunctionSubop::htNoKeyLookup(this, agg_pointer_result, *granules[0].first, hash_table));
   }

   // Step 3: Update the aggregation state.
   // The current offset into the serialized aggregate state. The aggregate state starts after the serialized key.
   size_t curr_offset = key_size + payload_offset;
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
   // Dispatch the correct reader depending on the layout.
   if (requires_complex_ht) {
      read_pipe.attachSuboperator(ComplexHashTableSource::build(this, ht_scan_result, static_cast<HashTableComplexKey*>(hash_table->access(0))));
   } else if (key_size == 2) {
      read_pipe.attachSuboperator(DirectLookupHashTableSource::build(this, ht_scan_result, static_cast<HashTableDirectLookup*>(hash_table->access(0))));
   } else {
      read_pipe.attachSuboperator(SimpleHashTableSource::build(this, ht_scan_result, static_cast<HashTableSimpleKey*>(hash_table->access(0))));
   }

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
