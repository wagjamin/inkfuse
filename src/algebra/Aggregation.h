#ifndef INKFUSE_AGGREGATION_H
#define INKFUSE_AGGREGATION_H

#include "algebra/AggFunctionRegisty.h"
#include "algebra/RelAlgOp.h"

namespace inkfuse {

/// Relational algebra operator for aggregations.
struct Aggregation : public RelAlgOp {
   Aggregation(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string op_name_, std::vector<AggregateFunctions::Description> aggregates_);

   void decay(PipelineDAG& dag) const override;

   private:
   /// Plan the aggregation by splitting it into granules.
   void plan(std::vector<AggregateFunctions::Description> description);

   /// The granules that are used to update the internal aggregate state.
   std::vector<AggStatePtr> granules;
   /// The compute .
   std::vector<AggComputePtr> compute;
   std::vector<IU> out_ius;
};

}

#endif //INKFUSE_AGGREGATION_H
