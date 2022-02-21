#include "gflags/gflags.h"
#include "exec/ExecutionContext.h"

namespace {

} // namespace

using namespace inkfuse;

DEFINE_uint64(depth, 10, "Size of the expression tree");

// InkFuse main binary to run queries.
int main(int argc, char* argv[]) {
   gflags::SetUsageMessage("inkfuse_runner --depth <depth of expression tree>");
   gflags::ParseCommandLineFlags(&argc, &argv, true);

   uint64_t depth = FLAGS_depth;


   return 0;
}
