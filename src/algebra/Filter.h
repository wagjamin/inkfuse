#ifndef INKFUSE_FILTER_H
#define INKFUSE_FILTER_H

#include "algebra/RelAlgOp.h"

namespace inkfuse {

/// Relational filter on a given boolean input IU. For a filter on an expression, a previous
/// ExpressionOp needs to have constructed the expression result.
struct Filter : RelAlgOp {

   static std::unique_ptr<Filter> build(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string name, std::vector<const IU*> redefined_, const IU& filter_iu_);

   void decay(PipelineDAG& dag) const override;

   private:
   Filter(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string name, std::vector<const IU*> redefined_, const IU& filter_iu_);

   /// Output filter IU of boolean type.
   const IU& filter_iu;
   /// Pseudo IU passed between scoping and logic operator.
   IU pseudo_iu;
   /// IUs which have to be redefined.
   std::vector<const IU*> to_redefine;
   /// Redefined IUs after the filter.
   std::vector<IU> redefined;
};

}

#endif //INKFUSE_FILTER_H
