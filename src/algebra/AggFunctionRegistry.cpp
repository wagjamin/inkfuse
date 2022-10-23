#include "algebra/AggFunctionRegisty.h"
#include "algebra/suboperators/aggregation/AggComputeAvg.h"
#include "algebra/suboperators/aggregation/AggComputeUnpack.h"
#include "algebra/suboperators/aggregation/AggStateCount.h"
#include "algebra/suboperators/aggregation/AggStateSum.h"
#include "algebra/suboperators/row_layout/KeyUnpackerSubop.h"

namespace inkfuse::AggregateFunctions {

namespace {

/// Return a simple registry entry where reading from the aggregate granules is just
/// a simple key unpacking operation. In these cases the agg_reader subop will be
/// a regular key unpacker. This can be done for aggregates where the serialized aggregate
/// state is the same as the aggregation result (count, min, max, sum).
RegistryEntry computeAsUnpack(IR::TypeArc agg_state) {
   RegistryEntry result;
   result.result_type = agg_state;
   result.agg_reader = std::make_unique<AggComputeUnpack>(std::move(agg_state));
   return result;
}

RegistryEntry resolveCount(const IU& agg_iu) {
   auto result = computeAsUnpack(IR::SignedInt::build(8));
   result.granules.push_back(std::make_unique<AggStateCount>());
   return result;
}

RegistryEntry resolveSum(const IU& agg_iu) {
   auto result = computeAsUnpack(agg_iu.type);
   result.granules.push_back(std::make_unique<AggStateSum>(agg_iu.type));
   return result;
}

RegistryEntry resolveAvg(const IU& agg_iu) {
   RegistryEntry result;
   // Avg always returns a double in InkFuse.
   result.result_type = IR::Float::build(8);
   result.agg_reader = std::make_unique<AggComputeAvg>(agg_iu.type);
   // Two granules - one to compute the sum state and one to compute
   // the count state.
   result.granules.push_back(std::make_unique<AggStateSum>(agg_iu.type));
   result.granules.push_back(std::make_unique<AggStateCount>());
   return result;
}

}

RegistryEntry lookupSubops(const Description& description) {
   if (description.distinct) {
      throw std::runtime_error("Distinct aggregate functions not implemented.");
   } else {
      switch (description.code) {
         case Opcode::Count:
            return resolveCount(description.agg_iu);
         case Opcode::Sum:
            return resolveSum(description.agg_iu);
         case Opcode::Avg:
            return resolveAvg(description.agg_iu);
         case Opcode::Min:
         case Opcode::Max:
         case Opcode::Median:
            throw std::runtime_error("Aggregate function not implemented.");
      }
   }
}

}
