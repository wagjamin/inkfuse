#include "algebra/ExpressionOp.h"
#include "algebra/TableScan.h"
#include "algebra/suboperators/sinks/CountingSink.h"
#include "algebra/suboperators/sinks/FuseChunkSink.h"
#include "exec/ExecutionContext.h"
#include "exec/PipelineExecutor.h"
#include "gflags/gflags.h"
#include "interpreter/FragmentCache.h"
#include "storage/Relation.h"
#include <chrono>
#include <deque>
#include <iostream>
#include <random>
#include <vector>

namespace {

} // namespace

using namespace inkfuse;

DEFINE_uint64(depth, 10, "Size of the expression tree");
DEFINE_double(sf, 1, "Data in the underlying three columns - in GB");

// InkFuse main binary to run queries.
// We are running on table T[a: int64_t, b: int64_t, c: uint32_t]
// Query: SELECT c:int64_t * (a + b) + c:int64_t * c:int64_t (a + b + c) + ...  FROM T
int main(int argc, char* argv[]) {
   gflags::SetUsageMessage("inkfuse_runner --depth <depth of expression tree> --sf <sf>");
   gflags::ParseCommandLineFlags(&argc, &argv, true);

   uint64_t depth = FLAGS_depth;
   // double sf = FLAGS_sf;

   // Populate the fragment cache.
   FragmentCache::instance();

   std::vector<double> sfs{-5, -4, -3, -2, -1, 0, 1};
   for (auto ind : sfs) {
      auto sf = std::pow(10, ind);
      // Rows to get the required data size.
      uint64_t rows = sf * (1ull << 30) / 20;

      // Set up backing storage.
      StoredRelation rel;
      std::vector<uint64_t> max{20000, 4000, 10};
      std::vector<std::string> colnames{"a", "b", "c"};
      for (size_t col_id = 0; col_id < colnames.size(); ++col_id) {
         // Create column.
         auto& col = rel.attachTypedColumn<int64_t>(colnames[col_id]);
         col.getStorage().reserve(rows);

         // And fill it up.
         // std::random_device rd;
         std::mt19937 gen(41088322222);
         std::uniform_int_distribution<> distr(0, max[col_id]);
         for (uint64_t k = 0; k < rows; ++k) {
            col.getStorage().push_back(distr(gen));
         }
      }

      // Set up the expression tree.
      std::deque<IU> in_ius;

      // Set up the table scan.
      std::unique_ptr<RelAlgOp> scan = std::make_unique<TableScan>(rel, std::vector<std::string>{"a", "b", "c"}, "tscan");
      auto ius = scan->getIUs();
      auto& in_0 = ius[0];
      auto& in_1 = ius[1];
      auto& in_2 = ius[2];

      // Set up the expression tree.
      std::vector<ExpressionOp::NodePtr> nodes;
      nodes.reserve(5);
      auto c1 = nodes.emplace_back(std::make_unique<ExpressionOp::IURefNode>(in_0)).get();
      auto c2 = nodes.emplace_back(std::make_unique<ExpressionOp::IURefNode>(in_1)).get();
      auto c3 = nodes.emplace_back(std::make_unique<ExpressionOp::IURefNode>(in_2)).get();
      auto c4 = nodes.emplace_back(
                        std::make_unique<ExpressionOp::ComputeNode>(
                           ExpressionOp::ComputeNode::Type::Add,
                           std::vector<ExpressionOp::Node*>{c1, c2}))
                   .get();
      auto c5 = nodes.emplace_back(
                        std::make_unique<ExpressionOp::ComputeNode>(
                           IR::SignedInt::build(8),
                           c3))
                   .get();
      auto c6 = nodes.emplace_back(std::make_unique<ExpressionOp::ComputeNode>(
                                      ExpressionOp::ComputeNode::Type::Subtract,
                                      std::vector<ExpressionOp::Node*>{c4, c5}))
                   .get();
      auto c7 = nodes.emplace_back(std::make_unique<ExpressionOp::ComputeNode>(
                                      ExpressionOp::ComputeNode::Type::Multiply,
                                      std::vector<ExpressionOp::Node*>{c5, c6}))
                   .get();

      std::vector<std::unique_ptr<RelAlgOp>> children;
      children.push_back(std::move(scan));

      ExpressionOp op(
         std::move(children),
         "expression_1",
         std::vector<ExpressionOp::Node*>{c7},
         std::move(nodes));

      /// Set up the actual pipelines.
      PipelineDAG dag;
      dag.buildNewPipeline();
      op.decay({}, dag);

      auto& pipe = dag.getCurrentPipeline();
      auto expression_result_iu = op.getIUs().back();
      auto& sink = pipe.attachSuboperator(FuseChunkSink::build(nullptr, *expression_result_iu));

      auto runInMode = [&](PipelineExecutor::ExecutionMode mode, std::string readable) {
         // Get ready for compiled execution.
         PipelineExecutor exec(pipe, mode, "ExpressionT_exec");

         auto start = std::chrono::steady_clock::now();
         exec.runPipeline();
         auto stop = std::chrono::steady_clock::now();
         auto dur = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();

         auto state = reinterpret_cast<CountingState*>(sink.accessState());
         if (state->count != rows) {
            // throw std::runtime_error("Did not calculate expression on all rows.");
         }

         std::cout << ind << "," << rows << "," << readable << "," << dur << std::endl;
      };

      runInMode(PipelineExecutor::ExecutionMode::Fused, "Fused");
      runInMode(PipelineExecutor::ExecutionMode::Fused, "Fused");
      runInMode(PipelineExecutor::ExecutionMode::Fused, "Fused");
      runInMode(PipelineExecutor::ExecutionMode::Interpreted, "Interpreted");
      runInMode(PipelineExecutor::ExecutionMode::Interpreted, "Interpreted");
      runInMode(PipelineExecutor::ExecutionMode::Interpreted, "Interpreted");
      runInMode(PipelineExecutor::ExecutionMode::Hybrid, "Hybrid");
      runInMode(PipelineExecutor::ExecutionMode::Hybrid, "Hybrid");
      runInMode(PipelineExecutor::ExecutionMode::Hybrid, "Hybrid");
   }

   return 0;
}
