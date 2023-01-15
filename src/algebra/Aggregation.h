#ifndef INKFUSE_AGGREGATION_H
#define INKFUSE_AGGREGATION_H

#include "algebra/AggFunctionRegisty.h"
#include "algebra/RelAlgOp.h"
#include <list>
#include <optional>

namespace inkfuse {

/// Relational algebra operator for aggregations.
struct Aggregation : public RelAlgOp {
   Aggregation(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string op_name_, std::vector<const IU*> group_by_, std::vector<AggregateFunctions::Description> aggregates_);
   static std::unique_ptr<Aggregation> build(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string op_name_, std::vector<const IU*> group_by_, std::vector<AggregateFunctions::Description> aggregates_);

   void decay(PipelineDAG& dag) const override;

   private:
   /// Plan the aggregation by splitting it into granules.
   void plan(std::vector<AggregateFunctions::Description> description);

   /// A planned aggregate computation.
   struct PlannedAggCompute {
      /// The actual computation.
      AggComputePtr compute;
      /// Offsets of the granules within the packed hash table payload.
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
   /// Char[]-typed IU to represent the optional packed key of the hash table.
   /// Need an optional as we need to plan the key layout before we can initialize it.
   std::optional<IU> packed_ht_key;
   /// Char*-typed IU to get the result of a hash table lookup/insert.
   IU agg_pointer_result;
   /// Char*-typed IU produced by reading from the hash table.
   IU ht_scan_result;
   /// Size of the aggregation key.
   size_t key_size = 0;
   /// Offset of the payload - i.e. after how many bytes does the first granule start.
   size_t payload_offset = 0;
   /// Size of the aggregation payload.
   size_t payload_size = 0;
   /// Does this aggregation require a complex hash table?
   bool requires_complex_ht = false;
};

}

#endif //INKFUSE_AGGREGATION_H
