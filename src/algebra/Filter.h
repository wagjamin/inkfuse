#ifndef INKFUSE_FILTER_H
#define INKFUSE_FILTER_H

#include "algebra/RelAlgOp.h"

namespace inkfuse {

/// Relational filter on a given boolean input IU. For a filter on an expression, a previous
/// ExpressionOp needs to have constructed the expression result.
struct Filter : RelAlgOp {

   Filter(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string name, const IU& filter_iu_);

   void decay(std::unordered_set<const IU*> required, PipelineDAG& dag) const override;

   void addIUs(std::unordered_set<const IU*>& set) const override;

   private:
   /// Output filter IU of boolean type.
   const IU& filter_iu;
};

}

#endif //INKFUSE_FILTER_H
