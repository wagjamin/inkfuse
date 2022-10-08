#ifndef INKFUSE_AGGREGATION_H
#define INKFUSE_AGGREGATION_H

#include "algebra/AggFunctionRegisty.h"
#include "algebra/RelAlgOp.h"
#include <list>

namespace inkfuse {

/// Relational algebra operator for aggregations.
struct Aggregation : public RelAlgOp {
   Aggregation(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string op_name_, std::vector<const IU*> group_by_, std::vector<AggregateFunctions::Description> aggregates_);

   void decay(PipelineDAG& dag) const override;

   private:
   /// Plan the aggregation by splitting it into granules.
   void plan(std::vector<AggregateFunctions::Description> description);

   /// A planned aggregate computation.
   struct PlannedAggCompute {
      /// The actual computation.
      AggComputePtr compute;
      /// Granule offset of the arguments in the backing `granules` vector.
      std::vector<size_t> granule_offsets;
   };

   /// By what should we aggregate?
   std::vector<const IU*> group_by;
   /// The granules that are used to update the internal aggregate state.
   std::vector<std::pair<const IU*, AggStatePtr>> granules;
   /// The compute granules that will be turned into AggReaderSubops.
   std::vector<PlannedAggCompute> compute;
   /// The output IUs for the aggregated keys.
   std::list<IU> out_key_ius;
   /// The result IUs that are produced by the aggregate functions.
   std::list<IU> out_aggregate_ius;
   /// Void-typed pseudo-IUs that connect the key packing operators with the hash table insert. 
   std::list<IU> pseudo_ius;
   /// Char*-typed IU to get the result of a hash table lookup/insert.
   IU agg_pointer_result;
   /// Char*-typed IU produced by reading from the hash table.
   IU ht_scan_result;
   /// Size of the aggregation key.
   size_t key_size = 0;
   /// Size of the aggregation payload.
   size_t payload_size = 0;
};

}

#endif //INKFUSE_AGGREGATION_H
