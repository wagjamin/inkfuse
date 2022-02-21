#include "gflags/gflags.h"
#include "exec/ExecutionContext.h"
#include "exec/PipelineExecutor.h"
#include "algebra/ExpressionOp.h"
#include "algebra/TableScan.h"
#include "storage/Relation.h"
#include <iostream>
#include <random>
#include <deque>

namespace {

} // namespace

using namespace inkfuse;

DEFINE_uint64(depth, 10, "Size of the expression tree");
DEFINE_uint64(sf, 1, "Data in the underlying three columns - in GB");

// InkFuse main binary to run queries.
// We are running on table T[a: int64_t, b: int64_t, c: uint32_t]
// Query: SELECT c:int64_t * (a + b) + c:int64_t * c:int64_t (a + b + c) + ...  FROM T
int main(int argc, char* argv[]) {
   gflags::SetUsageMessage("inkfuse_runner --depth <depth of expression tree> --sf <sf>");
   gflags::ParseCommandLineFlags(&argc, &argv, true);

   uint64_t depth = FLAGS_depth;
   uint64_t sf = FLAGS_sf;

   // Rows to get the required data size.
   uint64_t rows = sf * (1ull<<30) / 20;
   std::cout << "Running on " << rows << " rows.\n";

   // Set up backing storage.
   StoredRelation rel;
   std::vector<uint64_t> max{100, 1400, 43334};
   std::vector<std::string> colnames{"a", "b", "c"};
   for (size_t col_id = 0; col_id < colnames.size(); ++col_id) {
      // Create column.
      auto& col = rel.attachTypedColumn<uint64_t>(colnames[col_id]);
      col.getStorage().reserve(rows);

      // And fill it up.
      // std::random_device rd;
      std::mt19937 gen(41088322222);
      std::uniform_int_distribution<> distr(0, max[col_id]);
      for (uint64_t k = 0; k < rows; ++k)
      {
         col.getStorage().push_back(distr(gen));
      }
   }


   // Set up the expression tree.
   std::deque<IU> in_ius;

   // Set up the table scan.
   TableScan scan(rel, {"a", "b", "c"}, "tscan");
   auto ius = scan.getIUs();
   auto& in_0 = ius[0];
   auto& in_1 = ius[1];
   auto& in_2 = ius[2];

   // Set up the expression tree.
   std::vector<ExpressionOp::NodePtr> nodes;
   for (uint64_t k = 0; k < depth; ++k) {
      
   }


   return 0;
}
