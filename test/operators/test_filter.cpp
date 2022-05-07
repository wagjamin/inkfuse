#include "algebra/CompilationContext.h"
#include "algebra/ExpressionOp.h"
#include "algebra/Filter.h"
#include "algebra/Pipeline.h"
#include "algebra/RelAlgOp.h"
#include "algebra/suboperators/ExpressionSubop.h"
#include "codegen/backend_c/BackendC.h"
#include "exec/PipelineExecutor.h"
#include <gtest/gtest.h>

namespace inkfuse {

namespace {

/// Test fixture setting up a simple filter expression.
struct FilterT : ::testing::Test {

   FilterT():
      in1(IR::UnsignedInt::build(4), "in_1"),
      in2(IR::UnsignedInt::build(8), "in_2")
   {
      nodes.reserve(3);
      auto c1 = nodes.emplace_back(std::make_unique<ExpressionOp::IURefNode>(&in1)).get();
      auto c2 = nodes.emplace_back(std::make_unique<ExpressionOp::IURefNode>(&in2)).get();
      auto c3 = nodes.emplace_back(
            std::make_unique<ExpressionOp::ComputeNode>(
               ExpressionOp::ComputeNode::Type::Less,
               std::vector<ExpressionOp::Node*>{c1, c2}))
         .get();
      auto expression = std::make_unique<ExpressionOp>(
         std::vector<std::unique_ptr<RelAlgOp>>{},
         "expression_1",
         std::vector<ExpressionOp::Node*>{c3},
         std::move(nodes));
      const auto filter_iu = *expression->getIUs().begin();
      filter = std::make_unique<Filter>(std::vector<RelAlgOpPtr>{std::move(expression)}, "filter_1", *filter_iu);
   }

   /// Input columns.
   IU in1;
   IU in2;
   /// Expression computing in1 < in2.
   std::vector<ExpressionOp::NodePtr> nodes;
   /// Filter operator on the expression.
   RelAlgOpPtr filter;
};

}

}