#include "algebra/Aggregation.h"
#include "algebra/AggFunctionRegisty.h"
#include "algebra/Pipeline.h"
#include "algebra/suboperators/RuntimeFunctionSubop.h"
#include "algebra/suboperators/aggregation/AggReaderSubop.h"
#include "algebra/suboperators/aggregation/AggregatorSubop.h"
#include "algebra/suboperators/expressions/ExpressionSubop.h"
#include "algebra/suboperators/row_layout/KeyUnpackerSubop.h"
#include "algebra/suboperators/sources/HashTableSource.h"
#include <map>

namespace inkfuse {

Aggregation::Aggregation(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string op_name_, std::vector<const IU*> group_by_, std::vector<AggregateFunctions::Description> aggregates_)
   : RelAlgOp(std::move(children_), std::move(op_name_)), group_by(std::move(group_by_)),
     agg_pointer_result(IR::Pointer::build(IR::Char::build())), ht_scan_result(IR::Pointer::build(IR::Char::build())) {
   if (group_by.size() != 1) {
      // TODO(benjamin): Support aggregation by more keys and empty keys.
      throw std::runtime_error("Only single-column aggregation supported at the moment.");
   }
   plan(std::move(aggregates_));
}

std::unique_ptr<Aggregation> Aggregation::build(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string op_name_, std::vector<const IU*> group_by_, std::vector<AggregateFunctions::Description> aggregates_)
{
   return std::make_unique<Aggregation>(std::move(children_), std::move(op_name_), std::move(group_by_), std::move(aggregates_));
}

void Aggregation::plan(std::vector<AggregateFunctions::Description> description) {
   if (group_by.size() <= 1) {
      key_size += group_by.front()->type->numBytes();
   } else {
      // TODO(benjamin): Multi-agg
   }

   for (const auto& out_key_iu : group_by) {
      const auto& out = out_key_ius.emplace_back(out_key_iu->type);
      output_ius.push_back(&out);
   }

   // TODO(benjamin): Easy performance improvements:
   // - Compute granule layout ordered by IU
   // - Proper aligned state
   // The granules on a given IU that were computed already mapping to the offset in the 'granules' vector.
   std::map<std::pair<const IU*, std::string>, size_t> computed_granules;
   for (auto& func : description) {
      // Get the suboperators that make up this aggregate function.
      auto ops = AggregateFunctions::lookupSubops(func);
      std::vector<size_t> granule_offsets;
      granule_offsets.reserve(granules.size());
      for (auto& granule : ops.granules) {
         std::pair<const IU*, std::string> granule_id = {&func.agg_iu, granule->id()};
         if (computed_granules.count(granule_id) == 0) {
            // First time we see this granule, add it to the total set of granules.
            payload_size += granule->getStateSize();
            granules.emplace_back(&func.agg_iu, std::move(granule));
            computed_granules.emplace(granule_id, granules.size() - 1);
         } else {
            // We already computed this granule, reuse it.
            auto idx = computed_granules.at(granule_id);
            granule_offsets.push_back(idx);
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
   const IU* key_iu;
   if (group_by.size() > 1) {
      // Pack the aggregation key.
      // TODO(benjamin)
   } else {
      // There is only a single key, we can use that directly.
      key_iu = group_by[0];
   }

   // Step 2: Lookup or insert the key in the aggregation hash table
   auto& ht_lookup_subop = curr_pipe.attachSuboperator(RuntimeFunctionSubop::htLookupOrInsert(this, agg_pointer_result, *key_iu, {}, &hash_table));

   // Step 3: Update the aggregation state.
   // The current offset into the serialized aggregate state. The aggregate state starts after the serialized key.
   size_t curr_offset = key_size;
   for (const auto& [agg_iu, agg_state] : granules) {
      // Create the aggregator which will compute the granule.
      auto& aggregator = curr_pipe.attachSuboperator(AggregatorSubop::build(this, *agg_state, agg_pointer_result, *agg_iu));
      // Attach the runtime parameter that represents the state offset.
      KeyPackingRuntimeParams param;
      param.offsetSet(IR::UI<2>::build(curr_offset));
      reinterpret_cast<AggregatorSubop&>(aggregator).attachRuntimeParams(std::move(param));
      // Update the offset - the next aggregation state granule lives next to this one.
      curr_offset += agg_state->getStateSize();
   }

   // Step 4: Attach readers on a new pipeline.
   auto& read_pipe = dag.buildNewPipeline();
   // First, build a reader on the aggregate hash table returning pointers to the elements.
   read_pipe.attachSuboperator(HashTableSource::build(this, ht_scan_result, &hash_table));
   auto out_compute = compute.cbegin();

   // Produce the readers for the materialized keys.
   for (const auto& out_key_iu : out_key_ius) {
      auto& unpacker = read_pipe.attachSuboperator(KeyUnpackerSubop::build(this, ht_scan_result, out_key_iu));
      // Attach the runtime parameter that represents the state offset.
      KeyPackingRuntimeParams param;
      // TODO(benjamin) fix offset calculation.
      param.offsetSet(IR::UI<2>::build(0));
      reinterpret_cast<KeyUnpackerSubop&>(unpacker).attachRuntimeParams(std::move(param));
   }

   // Produce the actual operators that computes the aggregate functions.
   for (const auto& out_agg_iu : out_aggregate_ius) {
      auto& reader = read_pipe.attachSuboperator(AggReaderSubop::build(this, ht_scan_result, out_agg_iu, *out_compute->compute));
      // Attach the runtime parameter that represents the state offset.
      KeyPackingRuntimeParams param;
      // TODO(benjamin) fix offset calculation.
      param.offsetSet(IR::UI<2>::build(key_size));
      reinterpret_cast<AggReaderSubop&>(reader).attachRuntimeParams(std::move(param));
      out_compute++;
   }
}

}
