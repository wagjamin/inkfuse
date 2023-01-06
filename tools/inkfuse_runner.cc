#include "algebra/Print.h"
#include "common/Helpers.h"
#include "common/TPCH.h"
#include "exec/QueryExecutor.h"
#include "gflags/gflags.h"
#include "interpreter/FragmentCache.h"
#include "storage/Relation.h"
#include <chrono>
#include <deque>
#include <iostream>
#include <fstream>
#include <vector>

namespace {

using namespace inkfuse;

const std::string help = R"(inkfuse runner commands:
help - show this message
exit - exit the program
load sf<X> - load TPC-H data in "/sf<X>"
show - show what scale factor that got loaded
mute - don't show query text
unmute - do show query text
run q<N> [mode <ExecMode>] - run TPC-H query <N> on the loaded sf<X>
                             optional ExecMode in {Compiled, Interpreted, Hybrid}
                             default Hybrid.
)";

bool mute = false;

std::vector<std::string> splitCommand(const std::string& command) {
   std::vector<std::string> elems;
   std::istringstream input(command);
   std::string item;
   while (std::getline(input, item, ' ')) {
      elems.push_back(item);
   }
   return elems;
}

void runQuery(const std::string& q_name, std::unique_ptr<Print> root, PipelineExecutor::ExecutionMode mode) {
   std::ifstream input("q/" + q_name + ".sql");
   if (!mute && input.is_open()) {
      std::stringstream str;
      str << input.rdbuf();
      std::cout << "Running query\n"
                << str.str() << std::endl;
   }
   root->printer->setOstream(std::cout);
   PipelineDAG dag;
   root->decay(dag);
   auto start = std::chrono::steady_clock::now();
   QueryExecutor::runQuery(dag, mode, q_name);
   auto stop = std::chrono::steady_clock::now();
   std::cout << "Produced " << root->printer->num_rows << " rows after ";
   std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << "ms\n";
}

} // namespace

// InkFuse main binary to have some fun with queries.
int main(int argc, char* argv[]) {
   gflags::SetUsageMessage("inkfuse_runner");
   gflags::ParseCommandLineFlags(&argc, &argv, true);

   std::cout << "Starting Up ..." << std::endl;

   // Populate the fragment cache.
   FragmentCache::instance();

   std::cout << "Fragments Loaded - Ready to Go\n"
             << std::endl;

   // Only ingest data once and share it across tests.
   std::optional<Schema> loaded;
   std::string loaded_name;

   for (std::string command; std::getline(std::cin, command);) {
      std::vector<std::string> split = splitCommand(command);
      try {
         if (split.empty()) {
            continue;
         } else if (split[0] == "exit") {
            break;
         } else if (split[0] == "mute") {
            mute = true;
         } else if (split[0] == "unmute") {
            mute = false;
         } else if (split[0] == "help") {
            std::cout << help << std::endl;
         } else if (split[0] == "show") {
            if (loaded_name.empty()) {
               std::cout << "No data loaded" << std::endl;
            } else {
               std::cout << "Loaded scale factor " << loaded_name << std::endl;
            }
         } else if (split[0] == "load") {
            if (split.size() < 2) {
               std::cout << "invoke 'load' as 'load sf<X>'\n"
                         << std::endl;
            } else if (!loaded_name.empty()) {
               std::cout << "already loaded " << loaded_name
                         << std::endl;
            } else {
               std::cout << "Loading data at " << split[1] << std::endl;
               auto start = std::chrono::steady_clock::now();
               loaded = tpch::getTPCHSchema();
               helpers::loadDataInto(*loaded, split[1], true);
               auto end = std::chrono::steady_clock::now();
               std::cout << "Loaded after " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << " seconds" << std::endl;
               loaded_name = split[1];
            }
         } else if (split[0] == "run") {
            auto mode = PipelineExecutor::ExecutionMode::Hybrid;
            if (loaded_name.empty()) {
               std::cout << "You need to load data first through `load sf<X>`\n"
                         << std::endl;
            } else if (split.size() < 2) {
               std::cout << "invoke 'run' as 'run q<N> [mode {Compiled|Interpreted|Hybrid}]'\n"
                         << std::endl;
            } else {
               if (split.size() > 2 && split[2] == "mode") {
                  if (split[3] == "Compiled") {
                     mode = PipelineExecutor::ExecutionMode::Fused;
                  } else if (split[3] == "Interpreted") {
                     mode = PipelineExecutor::ExecutionMode::Interpreted;
                  } else if (split[3] == "Hybrid") {
                     mode = PipelineExecutor::ExecutionMode::Hybrid;
                  } else {
                     std::cout << "Unrecognized execution mode - we only support {Compiled|Interpreted|Hybrid}\n" << std::endl;
                     continue;
                  }
               }
               if (split[1] == "q1") {
                  auto q = tpch::q1(*loaded);
                  runQuery("1", std::move(q), mode);
               }
               else if (split[1] == "q3") {
                  auto q = tpch::q3(*loaded);
                  runQuery("3", std::move(q), mode);
               }
               else if (split[1] == "q6") {
                  auto q = tpch::q6(*loaded);
                  runQuery("6", std::move(q), mode);
               }
               else if (split[1] == "l_count") {
                  auto q = tpch::l_count(*loaded);
                  runQuery("l_count", std::move(q), mode);
               }
               else if (split[1] == "l_point") {
                  auto q = tpch::l_point(*loaded);
                  runQuery("l_point", std::move(q), mode);
               } else {
                  std::cout << "Unrecognized query - we only support {q1, q3, q6, l_count, l_point}\n";
               }
            }
         } else {
            std::cout << "Unknown command - type 'help' to see supported commands" << std::endl;
         }
      } catch (const std::exception& e) {
         std::cout << "Command failed with: " << e.what() << std::endl;
      }
      std::cout << std::endl;
   }

   return 0;
}
