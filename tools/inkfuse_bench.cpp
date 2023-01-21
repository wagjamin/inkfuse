#include "common/Helpers.h"
#include "common/TPCH.h"
#include "exec/QueryExecutor.h"
#include "gflags/gflags.h"
#include "interpreter/FragmentCache.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// String since it can be smaller than 1 - e.g. 0.01.
DEFINE_string(scale_factor, "1", "scale factor in the data directory");
DEFINE_int32(repetitions, 10, "how often each query should be run");

namespace {

using namespace inkfuse;

const std::string usage_message = R"(inkfuse benchmarking tool:
Run as ./inkfuse_bench <scale_factor> <repetitions>
Generates result files 'result_inkfuse_<mode>_<scale_factor>.csv'

We generate one file for each mode {Interpreted, Fused, Hybrid},
indicating the different execution backends.

Make sure data for ingest is loaded in 'data' directory in the script's working
directory. Clang-14 needs to be installed as it's used for code generation.

If you want to demo inkfuse in a more interactive way try out the inkfuse_runner
binary instead.
)";

/// The queries used for benchmarking.
const std::vector<std::pair<std::string, decltype(tpch::q1)*>> queries = {
   {"q1", tpch::q1},
   {"q3", tpch::q3},
   {"q4", tpch::q4},
   {"q6", tpch::q6},
   {"q14", tpch::q14},
   {"q18", tpch::q18},
   {"l_count", tpch::l_count},
   {"l_point", tpch::l_point},
};

/// The execution modes used for benchmarking.
const std::vector<std::pair<std::string, PipelineExecutor::ExecutionMode>> backends = {
   {"hybrid", PipelineExecutor::ExecutionMode::Hybrid},
   {"interpreted", PipelineExecutor::ExecutionMode::Interpreted},
   {"fused", PipelineExecutor::ExecutionMode::Fused},
};

}

int main(int argc, char* argv[]) {
   gflags::SetUsageMessage(usage_message);
   gflags::ParseCommandLineFlags(&argc, &argv, true);

   auto sf = FLAGS_scale_factor;
   auto reps = FLAGS_repetitions;

   // Populate the fragment cache.
   std::cout << "Generating & Loading Fragments ..." << std::endl;
   FragmentCache::instance();

   // Load data.
   std::cout << "Loading Data ..." << std::endl;
   auto schema = tpch::getTPCHSchema();
   {
      auto start = std::chrono::steady_clock::now();
      helpers::loadDataInto(schema, "data", true);
      auto end = std::chrono::steady_clock::now();
      std::cout << "Loaded after " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << " seconds" << std::endl;
   }

   // Benchmark each backend.
   for (const auto& [backend_name, backend_mode] : backends) {
      std::cout << "Benchmarking backend " << backend_name << "\n";
      // Measurements.
      std::vector<std::vector<size_t>> milliseconds(reps);

      // Run the queries.
      for (size_t k = 0; k < queries.size(); ++k) {
         auto& observations = milliseconds[k];
         observations.reserve(reps);
         const auto& [q_name, query_f] = queries[k];
         std::cout << "Benchmarking query " << q_name << "\n";
         for (int32_t rep = 0; rep < reps; ++rep) {
            auto root = query_f(schema);
            auto control_block = std::make_shared<PipelineExecutor::QueryControlBlock>(std::move(root));
            const auto start = std::chrono::steady_clock::now();
            QueryExecutor::runQuery(control_block, backend_mode, q_name + "_" + std::to_string(rep));
            const auto stop = std::chrono::steady_clock::now();
            const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            observations.push_back(millis);
         }
      }

      // Produce the result file.
      const std::string f_name = "result_inkfuse_" + backend_name + "_" + sf + ".csv";
      std::ofstream outfile;
      outfile.open(f_name);
      if (!outfile.is_open()) {
         throw std::runtime_error("Could not open result file " + f_name);
      }
      for (size_t k = 0; k < queries.size(); ++k) {
         const auto& [q_name, _] = queries[k];
         for (auto& measurement : milliseconds[k]) {
            outfile << "inkfuse_" << backend_name << "," << q_name << "," << sf << "," << measurement << "\n";
         }
      }
      outfile.close();
   }

   return 0;
}
