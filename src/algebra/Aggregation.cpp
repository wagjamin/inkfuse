#include "algebra/Aggregation.h"

namespace inkfuse {

Aggregation::Aggregation(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string op_name_, std::vector<AggregateFunctions::Description> aggregates_)
   : RelAlgOp(std::move(children_), std::move(op_name_)) {
   plan(std::move(aggregates_));
}

void Aggregation::plan(std::vector<AggregateFunctions::Description> description) {
}

void Aggregation::decay(PipelineDAG& dag) const {
}

}
