// InkFuse main binary to run queries on top of TPC-H
#include "gflags/gflags.h"
#include "exec/ExecutionContext.h"

namespace {

} // namespace

using namespace inkfuse;

int main(int argc, char* argv[]) {
   gflags::SetUsageMessage("inkfuse_runner");
   gflags::ParseCommandLineFlags(&argc, &argv, true);



   return 0;
}
