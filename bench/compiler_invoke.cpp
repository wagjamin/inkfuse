#include "benchmark/benchmark.h"
#include "exec/InterruptableJob.h"
#include <fstream>

/// The benchmarks in this file test the overhead of invoking the
/// C and C++ compilers in different ways.
/// We use a very simple toy program and a more sophisticated aggregation
/// query to be able to compare the constant overhead associated with the different approaches.
///
/// Overall invoking the compiler through the shell worked just fine
/// and makes it easy to pick up the compiler binary correctly. We thus
/// go with that.
namespace inkfuse {

namespace {

/// The programs we benchmark. For the complex one,
/// please make sure that you have generated the inkfuse runtime.
const std::vector<std::string> programs{"bench/testdata/simple_program.h", "bench/testdata/complex_program.h"};

void invoke_clang_bashed(benchmark::State& state) {
   const auto& prog = programs[state.range(0)];
   std::string cmd_str = "clang-14 -O3 -fPIC -shared " + prog;
   for (auto _ : state) {
      InterruptableJob interrupt;
      Command::runShell(cmd_str, interrupt);
   }
}

void invoke_clang_direct(benchmark::State& state) {
   const auto& prog = programs[state.range(0)];
   std::array<const char*, 6> command = {
      "/usr/bin/clang-14", "-O3", "-fPIC", "-shared", prog.data(), NULL};
   for (auto _ : state) {
      InterruptableJob interrupt;
      Command::run(command.data(), interrupt);
   }
}

void invoke_gcc_bashed(benchmark::State& state) {
   const auto& prog = programs[state.range(0)];
   std::string cmd_str = "gcc -O3 -fPIC -shared " + prog;
   for (auto _ : state) {
      InterruptableJob interrupt;
      Command::runShell(cmd_str, interrupt);
   }
}

void invoke_gcc_direct(benchmark::State& state) {
   const auto& prog = programs[state.range(0)];
   std::array<const char*, 6> command = {
      "/usr/bin/gcc", "-O3", "-fPIC", "-shared", prog.data(), NULL};
   for (auto _ : state) {
      InterruptableJob interrupt;
      Command::run(command.data(), interrupt);
   }
}

void invoke_clangpp_bashed(benchmark::State& state) {
   const auto& prog = programs[state.range(0)];
   std::string cmd_str = "clang++-14 -O3 -fPIC -shared " + prog;
   for (auto _ : state) {
      InterruptableJob interrupt;
      Command::runShell(cmd_str, interrupt);
   }
}

void invoke_clangpp_direct(benchmark::State& state) {
   const auto& prog = programs[state.range(0)];
   std::array<const char*, 6> command = {
      "/usr/bin/clang++-14", "-O3", "-fPIC", "-shared", prog.data(), NULL};
   for (auto _ : state) {
      InterruptableJob interrupt;
      Command::run(command.data(), interrupt);
   }
}

BENCHMARK(invoke_clang_bashed)->Arg(0)->Arg(1);
BENCHMARK(invoke_clang_direct)->Arg(0)->Arg(1);
BENCHMARK(invoke_clangpp_bashed)->Arg(0)->Arg(1);
BENCHMARK(invoke_clangpp_direct)->Arg(0)->Arg(1);
BENCHMARK(invoke_gcc_bashed)->Arg(0)->Arg(1);
BENCHMARK(invoke_gcc_direct)->Arg(0)->Arg(1);

}

}