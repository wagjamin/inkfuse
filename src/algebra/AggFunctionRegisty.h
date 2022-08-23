#ifndef INKFUSE_AGGFUNCTIONREGISTY_H
#define INKFUSE_AGGFUNCTIONREGISTY_H

#include "algebra/suboperators/Suboperator.h"
#include "algebra/suboperators/aggregation/AggCompute.h"
#include "algebra/suboperators/aggregation/AggState.h"
#include "codegen/Type.h"
#include <optional>

namespace inkfuse {

/// Central registry for the InkFuse aggregate functions. Given an aggregate description on a given type,
/// the AggFunctionRegistry resolves which aggregation suboperators are needed to compute the aggregate.
/// The computation of the aggregatestate is broken into the computation of smaller granules through
/// AggStates.
/// By returning AggStates directly rather than e.g. an AggregatorSubop the aggregation logic is able to
/// pick the smallest possible aggregation state in an elegant way (e.g. when computing count(x), avg(x)).
/// For the time being, this registry mixes logical- and physical concepts given that InkFuse doesn't have a planner.
namespace AggregateFunctions {

/// Which aggregate function should be implemented. Distinct combinator is provided
/// during lookups on the agg registry.
enum class Opcode {
   Min,
   Max,
   Sum,
   Count,
   Avg,
   Median,
};

/// High-level description of an aggreagte function.
struct Description {
   const IU& agg_iu;
   const Opcode code;
   const bool distinct;
};

/// Result of an agg function lookup.
struct RegistryEntry {
   /// Result type of the aggregation.
   IR::TypeArc result_type;
   /// Aggregation granules that have to be computed for the given aggregate function.
   std::vector<AggStatePtr> granules;
   /// Function that operates on the granules to compute the output of the aggregate function.
   /// Runtime params need to be attached to find state granule locations.
   AggComputePtr agg_reader;
};

/// Generate low-level computational description for the given aggregate.
RegistryEntry lookupSubops(const Description& description);

}

};

#endif //INKFUSE_AGGFUNCTIONREGISTY_H
