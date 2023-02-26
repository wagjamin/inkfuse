#include "PerfEvent.hpp"
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
DEFINE_bool(perf_events, false, "should we collect perf events for each query?");

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

   const auto sf = FLAGS_scale_factor;
   const auto reps = FLAGS_repetitions;
   const auto perf_events = FLAGS_perf_events;

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

   // If we are collecting perf events, set up the collector.
   BenchmarkParameters params;
   std::ofstream perf_outfile;
   if (perf_events) {
      const std::string perf_f_name = "inkfuse_bench_perf_result_" + sf + ".csv";
      perf_outfile.open(perf_f_name);
      if (!perf_outfile.is_open()) {
         throw std::runtime_error("Could not open result file " + perf_f_name);
      }
   }
   params.setParam("name", "inkfuse_microbenchmark");
   params.setParam("sf", sf);
   bool write_header = true;

   // Benchmark each backend.
   for (const auto& [backend_name, backend_mode] : backends) {
      params.setParam("backend", backend_name);
      std::cout << "Benchmarking backend " << backend_name << "\n";
      // Measurements. Pairs of (total_time <milliseconds>, compilation_stalled <microseconds>).
      std::vector<std::vector<std::pair<size_t, size_t>>> measurements(reps);

      // Run the queries.
      for (size_t k = 0; k < queries.size(); ++k) {
         auto& observations = measurements[k];
         observations.reserve(reps);
         const auto& [q_name, query_f] = queries[k];
         params.setParam("query", q_name);
         std::cout << "Benchmarking query " << q_name << "\n";
         for (int32_t rep = 0; rep < reps; ++rep) {
            auto root = query_f(schema);
            auto control_block = std::make_shared<PipelineExecutor::QueryControlBlock>(std::move(root));
            const auto start = std::chrono::steady_clock::now();
            PipelineExecutor::PipelineStats query_stats;
            if (perf_events) {
               // Split into compilation and execution as we don't want compilation to populate
               // perf metrics.
               QueryExecutor::StepwiseExecutor exec(control_block, backend_mode, q_name + "_" + std::to_string(rep));
               exec.prepareQuery();
               // Wait until compilation is definitely done.
               std::this_thread::sleep_for(std::chrono::milliseconds(200));
               // And now perfom the actual execution, measuring the CPU metrics.
               PerfEventBlock block(perf_outfile, 1, params, write_header);
               exec.runQuery();
               write_header = false;
            } else {
               query_stats = QueryExecutor::runQuery(control_block, backend_mode, q_name + "_" + std::to_string(rep));
            }
            const auto stop = std::chrono::steady_clock::now();
            const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            observations.emplace_back(millis, query_stats.codegen_microseconds);
         }
      }

      // Note: if perf_events is set, the result duration is unreliable since we sleep to let
      // code generation finish. Don't print the measurements in that case to not allow misuse.
      if (!perf_events) {
         // Produce the result file.
         const std::string f_name = "result_inkfuse_" + backend_name + "_" + sf + ".csv";
         std::ofstream outfile;
         outfile.open(f_name);
         if (!outfile.is_open()) {
            throw std::runtime_error("Could not open result file " + f_name);
         }
         for (size_t k = 0; k < queries.size(); ++k) {
            const auto& [q_name, _] = queries[k];
            for (auto& measurement : measurements[k]) {
               outfile << "inkfuse_" << backend_name << "," << q_name << "," << sf << "," << measurement.first << "," << measurement.second << "\n";
            }
         }
         outfile.close();
      }
   }

   return 0;
}
