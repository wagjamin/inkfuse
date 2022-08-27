#include "algebra/Aggregation.h"
#include "algebra/AggFunctionRegisty.h"
#include "algebra/Pipeline.h"
#include "algebra/suboperators/RuntimeFunctionSubop.h"
#include "algebra/suboperators/combinators/IfCombinator.h"
#include "algebra/suboperators/expressions/ExpressionSubop.h"
#include <map>

namespace inkfuse {

Aggregation::Aggregation(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string op_name_, std::vector<const IU*> group_by_, std::vector<AggregateFunctions::Description> aggregates_)
   : RelAlgOp(std::move(children_), std::move(op_name_)), group_by(std::move(group_by_)) {
   if (group_by_.size() != 1) {
      // TODO(benjamin): Support aggregation by more keys and empty keys.
      throw std::runtime_error("Only single-column aggregation supported at the moment.");
   }
   plan(std::move(aggregates_));
}

void Aggregation::plan(std::vector<AggregateFunctions::Description> description) {
   if (group_by.size() <= 1) {
      hash_ius.emplace_back(IR::UnsignedInt::build(8));
      key_size += group_by.front()->type->numBytes();
   } else {
      // TODO(benjamin): Multi-agg
   }
   // TODO(benjamin): Easy performance improvements:
   // - Compute granule layout ordered by IU
   // - Proper aligned state
   // The granules on a given IU that were computed already mapping to the offset in the 'granules' vector.
   std::map<std::pair<const IU*, std::string>, size_t> computed_granules;
   for (auto& func : description) {
      // Get the suboperators which make up this aggregate function.
      auto ops = AggregateFunctions::lookupSubops(func);
      std::vector<size_t> granule_offsets;
      granule_offsets.reserve(granules.size());
      for (auto& granule : granules) {
         std::pair<const IU*, std::string> granule_id = {&func.agg_iu, granule->id()};
         if (computed_granules.count(granule_id) == 0) {
            // First time we see this granule, add it to the total set of granules.
            payload_size += granule->getStateSize();
            granules.push_back(std::move(granule));
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
      auto& out_iu = out_ius.emplace_back(ops.result_type);
      output_ius.push_back(&out_iu);
   }
}

void Aggregation::decay(PipelineDAG& dag) const {
   for (const auto& child : children) {
      child->decay(dag);
   }

   // Decay the full aggregation operation into a DAG of suboperators.
   // This proceeds as-follows:
   // Current Pipeline:
   // 1. Hash the aggregation key
   // 2. Lookup the hash values in the aggregation hash table
   // 3. Resolve the hash collisions
   // 4. For new groups:
   //    4.1 Allocate a new group in the hash table
   //    4.2 Initialize the aggregate state
   // 5. For existing groups: update the aggregate state.
   // New Pipeline:
   // 6. Attach readers on a new pipeline.

   // Set up the backing aggregation hash table. We can drop it after the
   // pipeline which reads from the aggregation is done.
   // auto& hash_table = dag.attachHashTableSimpleKey(dag.getPipelines().size(), key_size, payload_size);

   // auto& curr_pipe = dag.getCurrentPipeline();
   // Step 1: Hash the aggregation columns.
   // curr_pipe.attachSuboperator(ExpressionSubop::build(this, {&hash_ius.front()}, {group_by.front()}, ExpressionOp::ComputeNode::Type::Hash));

   // Step 2: Lookup the hash values in the aggregation hash table
   // auto& ht_lookup_subop = curr_pipe.attachSuboperator(RuntimeFunctionSubop::htLookup(this, hash_ius.front(), &hash_table));

   // Step 3: Resolve the hash collisions.
   // TODO(Benjamin)

   // Step 4: Set up new groups where we did not find a key.
   // curr_pipe.attachSuboperator(IfCombinator<RuntimeFunctionSubop>::build({&ht_lookup_subop.getIUs().front()}, this, "ht_insert", hash_ius.front(), &hash_table));

   // Step 5: Update existing groups.

   // Step 6: Set up the new pipeline and readers.
   // auto& new_pipe = dag.buildNewPipeline();
}

}
