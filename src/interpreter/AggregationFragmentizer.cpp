#include "interpreter/AggregationFragmentizer.h"
#include "algebra/suboperators/aggregation/AggComputeUnpack.h"
#include "algebra/suboperators/aggregation/AggReaderSubop.h"
#include "algebra/suboperators/aggregation/AggStateCount.h"
#include "algebra/suboperators/aggregation/AggregatorSubop.h"

namespace inkfuse {

namespace {

const auto unpacking_types = TypeDecorator{}.attachTypes().produce();

}

AggregationFragmentizer::AggregationFragmentizer() {
   fragmentizeAggregators();
   fragmentizeAggReaders();
}

void AggregationFragmentizer::fragmentizeAggregators() {
   // Fragmentize count state over arbitrary types.
   auto& [name, pipe] = pipes.emplace_back();
   auto& state = agg_states.emplace_back(std::make_unique<AggStateCount>());
   auto& ptr_iu = generated_ius.emplace_back(IU{IR::Pointer::build(IR::Char::build())});
   auto& not_init_iu = generated_ius.emplace_back(IU{IR::Bool::build()});
   auto& agg_iu = generated_ius.emplace_back(IU{IR::SignedInt::build(8)});
   auto& op = pipe.attachSuboperator(AggregatorSubop::build(nullptr, *state, ptr_iu, not_init_iu, agg_iu));
   name = op.id();
}

void AggregationFragmentizer::fragmentizeAggReaders() {
   for (const auto& unpack_type: unpacking_types) {
      // Fragmentize agg state unpacking over all types.
      auto& [name, pipe] = pipes.emplace_back();
      auto& compute = agg_computes.emplace_back(std::make_unique<AggComputeUnpack>(unpack_type));
      auto& ptr_iu = generated_ius.emplace_back(IU{IR::Pointer::build(IR::Char::build())});
      auto& target_iu = generated_ius.emplace_back(unpack_type);
      auto& op = pipe.attachSuboperator(AggReaderSubop::build(nullptr, ptr_iu, target_iu, *compute));
      name = op.id();
   }
}

}