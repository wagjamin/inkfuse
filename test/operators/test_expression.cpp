#include "algebra/CompilationContext.h"
#include "algebra/ExpressionOp.h"
#include "algebra/Pipeline.h"
#include "algebra/RelAlgOp.h"
#include "codegen/Value.h"
#include "codegen/backend_c/BackendC.h"
#include "exec/PipelineExecutor.h"
#include <gtest/gtest.h>

namespace inkfuse {

namespace {

struct ExpressionT {
   ExpressionT() : in1(IR::UnsignedInt::build(2), "in_1"),
                   in2(IR::UnsignedInt::build(2), "in_2") {
      nodes.reserve(6);
      auto c1 = nodes.emplace_back(std::make_unique<ExpressionOp::IURefNode>(&in1)).get();
      auto c2 = nodes.emplace_back(std::make_unique<ExpressionOp::IURefNode>(&in2)).get();
      auto c3 = nodes.emplace_back(
                        std::make_unique<ExpressionOp::ComputeNode>(
                           ExpressionOp::ComputeNode::Type::Add,
                           std::vector<ExpressionOp::Node*>{c1, c2}))
                   .get();
      auto c4 = nodes.emplace_back(std::make_unique<ExpressionOp::ComputeNode>(
                                      ExpressionOp::ComputeNode::Type::Subtract,
                                      std::vector<ExpressionOp::Node*>{c3, c2}))
                   .get();
      auto c5 = nodes.emplace_back(std::make_unique<ExpressionOp::ComputeNode>(
                                      ExpressionOp::ComputeNode::Type::Multiply,
                                      std::vector<ExpressionOp::Node*>{c3, c4}))
                   .get();
      // Finally addition with a constant which will produce a RuntimeExpressionOp.
      auto c6 = nodes.emplace_back(std::make_unique<ExpressionOp::ComputeNode>(
                                      ExpressionOp::ComputeNode::Type::Add,
                                      IR::UI<2>::build(5), c5))
                   .get();
      op.emplace(
         std::vector<std::unique_ptr<RelAlgOp>>{},
         "expression_1",
         std::vector<ExpressionOp::Node*>{c3, c5, c6},
         std::move(nodes));
   }

   IU in1;
   IU in2;
   std::vector<ExpressionOp::NodePtr> nodes;
   std::optional<ExpressionOp> op;
};

/// Non-parametrized test fixture for decay tests.
struct ExpressionTNonParametrized : public ExpressionT, public ::testing::Test {
};

/// Parametrized test fixture for execution modes.
struct ExpressionTParametrized : public ExpressionT, public ::testing::TestWithParam<PipelineExecutor::ExecutionMode> {
};

// Check graph structure for expression decay.
TEST_F(ExpressionTNonParametrized, decay) {
   PipelineDAG dag;
   dag.buildNewPipeline();
   op->decay(dag);

   // One pipeline.
   EXPECT_EQ(dag.getPipelines().size(), 1);

   auto& pipe = dag.getCurrentPipeline();
   auto& ops = pipe.getSubops();
   // Four actual computations are done.
   EXPECT_EQ(ops.size(), 4);
   EXPECT_EQ(ops[0]->getSourceIUs()[0], &in1);
   EXPECT_EQ(ops[0]->getSourceIUs()[1], &in2);
   EXPECT_EQ(pipe.getConsumers(*ops[0]).size(), 2);
   EXPECT_EQ(pipe.getConsumers(*ops[1]).size(), 1);
   // Only one input on runtime expression op.
   EXPECT_EQ(pipe.getProducers(*ops[3]).size(), 1);
}

TEST_P(ExpressionTParametrized, exec) {
   PipelineDAG dag;
   dag.buildNewPipeline();
   op->decay(dag);

   auto& pipe = dag.getCurrentPipeline();
   // Repipe to add fuse chunk sinks and sources.
   auto repiped = pipe.repipeAll(0, pipe.getSubops().size());

   // Repiping should have added 1 driver, 2 iu input and 4 iu output ops.
   auto& ops = repiped->getSubops();
   EXPECT_EQ(ops.size(), 11);

   // Get ready for compiled execution.
   auto mode = GetParam();
   PipelineExecutor exec(*repiped, mode, "ExpressionT_exec");

   // Prepare input chunk.
   auto& ctx = exec.getExecutionContext();
   auto& c_in1 = ctx.getColumn(in1);
   auto& c_in2 = ctx.getColumn(in2);

   c_in1.size = 10;
   c_in2.size = 10;
   for (uint16_t k = 0; k < 10; ++k) {
      reinterpret_cast<uint16_t*>(c_in1.raw_data)[k] = k + 1;
      reinterpret_cast<uint16_t*>(c_in2.raw_data)[k] = k;
   }

   // And run a single morsel.
   EXPECT_NO_THROW(exec.runMorsel());

   // Check results.
   auto iu_c_3 = *pipe.getSubops()[0]->getIUs().begin();
   auto iu_c_4 = *pipe.getSubops()[1]->getIUs().begin();
   auto iu_c_5 = *pipe.getSubops()[2]->getIUs().begin();
   auto iu_c_6 = *pipe.getSubops()[3]->getIUs().begin();
   auto& c_iu_c_3 = ctx.getColumn(*iu_c_3);
   auto& c_iu_c_4 = ctx.getColumn(*iu_c_4);
   auto& c_iu_c_5 = ctx.getColumn(*iu_c_5);
   auto& c_iu_c_6 = ctx.getColumn(*iu_c_6);
   for (uint16_t k = 0; k < 10; ++k) {
      uint16_t c_3_expected = k + k + 1;
      uint16_t c_4_expected = k + 1;
      uint16_t c_5_expected = c_3_expected * c_4_expected;
      uint16_t c_6_expected = c_5_expected + 5;
      EXPECT_EQ(reinterpret_cast<uint16_t*>(c_iu_c_3.raw_data)[k], c_3_expected);
      EXPECT_EQ(reinterpret_cast<uint16_t*>(c_iu_c_4.raw_data)[k], c_4_expected);
      EXPECT_EQ(reinterpret_cast<uint16_t*>(c_iu_c_5.raw_data)[k], c_5_expected);
      EXPECT_EQ(reinterpret_cast<uint16_t*>(c_iu_c_6.raw_data)[k], c_6_expected);
   }
}

// Test hashing expressions to make sure they work.
TEST_P(ExpressionTParametrized, hash) {
   // Custom setup going over a different expression tree.
   nodes.clear();
   IU source(IR::UnsignedInt::build(8));
   auto ref = nodes.emplace_back(std::make_unique<ExpressionOp::IURefNode>(&source)).get();
   auto hash = nodes.emplace_back(
                       std::make_unique<ExpressionOp::ComputeNode>(
                          ExpressionOp::ComputeNode::Type::Hash,
                          std::vector<ExpressionOp::Node*>{ref}))
                  .get();
   op.emplace(
      std::vector<std::unique_ptr<RelAlgOp>>{},
      "expression_1",
      std::vector<ExpressionOp::Node*>{hash},
      std::move(nodes));

   PipelineDAG dag;
   dag.buildNewPipeline();
   op->decay(dag);

   auto& pipe = dag.getCurrentPipeline();
   // Repipe to add fuse chunk sinks and sources.
   auto repiped = pipe.repipeAll(0, pipe.getSubops().size());

   // Repiping should have added 1 driver, 1 iu input and 1 output op.
   auto& ops = repiped->getSubops();
   EXPECT_EQ(ops.size(), 4);

   // Get ready for compiled execution.
   auto mode = GetParam();
   PipelineExecutor exec(*repiped, mode, "ExpressionT_hash");

   // Prepare input chunk.
   auto& ctx = exec.getExecutionContext();
   auto& c_in1 = ctx.getColumn(source);

   c_in1.size = 1000;
   for (uint16_t k = 0; k < 1000; ++k) {
      reinterpret_cast<uint64_t*>(c_in1.raw_data)[k] = k;
   }

   // And run a single morsel.
   EXPECT_NO_THROW(exec.runMorsel());

   // Check results.
   const IU& hash_iu = **ops[2]->getIUs().begin();
   std::unordered_set<uint64_t> seen;
   // This set should have no hash collisions.
   auto& hash_col = ctx.getColumn(hash_iu);
   for (uint16_t k = 0; k < 1000; ++k) {
      auto elem = reinterpret_cast<uint64_t*>(hash_col.raw_data)[k];
      EXPECT_EQ(seen.count(elem), 0);
      seen.insert(elem);
   }
}

INSTANTIATE_TEST_CASE_P(
   ExpressionExecution,
   ExpressionTParametrized,
   ::testing::Values(PipelineExecutor::ExecutionMode::Fused,
                     PipelineExecutor::ExecutionMode::Interpreted));

}

}