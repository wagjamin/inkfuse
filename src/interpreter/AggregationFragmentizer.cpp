#include "interpreter/AggregationFragmentizer.h"
#include "algebra/suboperators/aggregation/AggComputeAvg.h"
#include "algebra/suboperators/aggregation/AggComputeUnpack.h"
#include "algebra/suboperators/aggregation/AggReaderSubop.h"
#include "algebra/suboperators/aggregation/AggStateCount.h"
#include "algebra/suboperators/aggregation/AggStateSum.h"
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
   auto& agg_iu = generated_ius.emplace_back(IU{IR::SignedInt::build(8)});
   auto& op = pipe.attachSuboperator(AggregatorSubop::build(nullptr, *state, ptr_iu, agg_iu));
   name = op.id();

   // Fragmentize sum over all numeric types.
   for (const auto& type : TypeDecorator{}.attachNumeric().produce()) {
      auto& [name, pipe] = pipes.emplace_back();
      auto& state = agg_states.emplace_back(std::make_unique<AggStateSum>(type));
      auto& ptr_iu = generated_ius.emplace_back(IU{IR::Pointer::build(IR::Char::build())});
      auto& agg_iu = generated_ius.emplace_back(IU{type});
      auto& op = pipe.attachSuboperator(AggregatorSubop::build(nullptr, *state, ptr_iu, agg_iu));
      name = op.id();
   }
}

void AggregationFragmentizer::fragmentizeAggReaders() {
   // Fragmentize agg state unpacking over all types.
   for (const auto& unpack_type: unpacking_types) {
      auto& [name, pipe] = pipes.emplace_back();
      auto& compute = agg_computes.emplace_back(std::make_unique<AggComputeUnpack>(unpack_type));
      auto& ptr_iu = generated_ius.emplace_back(IU{IR::Pointer::build(IR::Char::build())});
      auto& target_iu = generated_ius.emplace_back(unpack_type);
      auto& op = pipe.attachSuboperator(AggReaderSubop::build(nullptr, ptr_iu, target_iu, *compute));
      name = op.id();
   }

   // Fragmentize average unpacking over all numeric types.
   for (const auto& type : TypeDecorator{}.attachNumeric().produce()) {
      auto& [name, pipe] = pipes.emplace_back();
      auto& compute = agg_computes.emplace_back(std::make_unique<AggComputeAvg>(type));
      auto& ptr_iu = generated_ius.emplace_back(IU{IR::Pointer::build(IR::Char::build())});
      auto& target_iu = generated_ius.emplace_back(IR::Float::build(8));
      auto& op = pipe.attachSuboperator(AggReaderSubop::build(nullptr, ptr_iu, target_iu, *compute));
      name = op.id();
   }
}

}