// InkFuse main binary to run queries on top of TPC-H
#include "gflags/gflags.h"

namespace {

} // namespace

DEFINE_string(include_dir, "", "Path to imlabdb include directories");

static bool ValidateFlag(const char* flagname, const std::string& value) {
   return !value.empty();
}

DEFINE_validator(include_dir, &ValidateFlag);

int main(int argc, char* argv[]) {
   gflags::SetUsageMessage("imlabdb --include_dir <path to imlab include dir>");
   gflags::ParseCommandLineFlags(&argc, &argv, true);

   return 0;
}
