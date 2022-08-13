#ifndef INKFUSE_AGGREGATIONFRAGMENTIZER_H
#define INKFUSE_AGGREGATIONFRAGMENTIZER_H

#include "algebra/suboperators/aggregation/AggCompute.h"
#include "algebra/suboperators/aggregation/AggState.h"
#include "interpreter/FragmentGenerator.h"

namespace inkfuse {

struct AggregationFragmentizer : public Fragmentizer {
   AggregationFragmentizer();

   private:
   /// Fragmentize all aggregators.
   void fragmentizeAggregators();
   /// Fragmentize all aggregation readers.
   void fragmentizeAggReaders();

   /// Internal state for the created aggregation suboperators.
   std::vector<AggStatePtr> agg_states;
   std::vector<AggComputePtr> agg_computes;
};

}

#endif //INKFUSE_AGGREGATIONFRAGMENTIZER_H
