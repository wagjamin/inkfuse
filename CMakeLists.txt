# ---------------------------------------------------------------------------
# InkFuse - An Incremental Fusion engine unifying compiled and vectorized query execution.
# ---------------------------------------------------------------------------

project(inkfuse)
cmake_minimum_required(VERSION 3.12)
# ---------------------------------------------------------------------------
# Environment
# ---------------------------------------------------------------------------

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -rdynamic -g -O0 -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

find_package(Threads REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)

include("${CMAKE_SOURCE_DIR}/vendor/googletest.cmake")
include("${CMAKE_SOURCE_DIR}/vendor/gflags.cmake")
include("${CMAKE_SOURCE_DIR}/vendor/xxhash.cmake")

option(WITH_COVERAGE "Build inkfuse with coverage information attached" OFF)
if (WITH_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage")
endif ()

option(JIT_CLANG_11 "Use clang-11 as the default compiler. If off, compiler that should be used is read from CUSTOM_JIT env variable" ON)
if (JIT_CLANG_11)
    add_compile_definitions(WITH_JIT_CLANG_11)
endif ()

# ---------------------------------------------------------------------------
# Includes
# ---------------------------------------------------------------------------

include_directories(
        ${CMAKE_SOURCE_DIR}/thirdparty
        ${CMAKE_SOURCE_DIR}/src
        ${XXHASH_INCLUDE_DIR}
        ${GTEST_INCLUDE_DIR}
        ${GMOCK_INCLUDE_DIR}
        ${GFLAGS_INCLUDE_DIR}
)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------

set(INCLUDE_H
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/Copy.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/Suboperator.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/IndexedIUProvider.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/CountingSink.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/ColumnFilter.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/RuntimeFunctionSubop.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/LoopDriver.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/RuntimeParam.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/aggregation/AggCompute.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/aggregation/AggComputeUnpack.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/aggregation/AggReaderSubop.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/aggregation/AggregatorSubop.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/aggregation/AggState.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/aggregation/AggStateCount.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/combinators/IfCombinator.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/expressions/ExpressionHelpers.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/expressions/ExpressionSubop.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/expressions/RuntimeExpressionSubop.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/expressions/RuntimeKeyExpressionSubop.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/properties/WithRuntimeParams.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/row_layout/KeyPackerSubop.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/row_layout/KeyPackingRuntimeState.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/row_layout/KeyUnpackerSubop.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/sinks/FuseChunkSink.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/sinks/CountingSink.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/sources/TableScanSource.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/sources/HashTableSource.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/sources/FuseChunkSource.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/sources/ScratchPadIUProvider.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/Pipeline.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/RelAlgOp.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/ExpressionOp.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/Filter.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/IU.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/TableScan.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/Aggregation.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/AggFunctionRegistry.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/CompilationContext.h"
        "${CMAKE_SOURCE_DIR}/src/codegen/backend_c/BackendC.h"
        "${CMAKE_SOURCE_DIR}/src/codegen/backend_c/ScopedWriter.h"
        "${CMAKE_SOURCE_DIR}/src/codegen/Backend.h"
        "${CMAKE_SOURCE_DIR}/src/codegen/Expression.h"
        "${CMAKE_SOURCE_DIR}/src/codegen/IR.h"
        "${CMAKE_SOURCE_DIR}/src/codegen/IRBuilder.h"
        "${CMAKE_SOURCE_DIR}/src/codegen/Type.h"
        "${CMAKE_SOURCE_DIR}/src/common/Helpers.h"
        "${CMAKE_SOURCE_DIR}/src/common/QueriesTPCH.h"
        "${CMAKE_SOURCE_DIR}/src/exec/FuseChunk.h"
        "${CMAKE_SOURCE_DIR}/src/exec/PipelineExecutor.h"
        "${CMAKE_SOURCE_DIR}/src/exec/QueryExecutor.h"
        "${CMAKE_SOURCE_DIR}/src/exec/runners/PipelineRunner.h"
        "${CMAKE_SOURCE_DIR}/src/exec/runners/CompiledRunner.h"
        "${CMAKE_SOURCE_DIR}/src/exec/runners/InterpretedRunner.h"
        "${CMAKE_SOURCE_DIR}/src/exec/ExecutionContext.h"
        "${CMAKE_SOURCE_DIR}/src/exec/InterpretationResult.h"
        "${CMAKE_SOURCE_DIR}/src/exec/InterruptableJob.h"
        "${CMAKE_SOURCE_DIR}/src/interpreter/AggregationFragmentizer.h"
        "${CMAKE_SOURCE_DIR}/src/interpreter/ColumnFilterFragmentizer.h"
        "${CMAKE_SOURCE_DIR}/src/interpreter/FragmentCache.h"
        "${CMAKE_SOURCE_DIR}/src/interpreter/FragmentGenerator.h"
        "${CMAKE_SOURCE_DIR}/src/interpreter/HashTableSourceFragmentizer.h"
        "${CMAKE_SOURCE_DIR}/src/interpreter/ExpressionFragmentizer.h"
        "${CMAKE_SOURCE_DIR}/src/interpreter/CountingSinkFragmentizer.h"
        "${CMAKE_SOURCE_DIR}/src/interpreter/RuntimeKeyExpressionFragmentizer.h"
        "${CMAKE_SOURCE_DIR}/src/interpreter/RuntimeFunctionSubopFragmentizer.h"
        "${CMAKE_SOURCE_DIR}/src/interpreter/KeyPackingFragmentizer.h"
        "${CMAKE_SOURCE_DIR}/src/interpreter/TScanFragmentizer.h"
        "${CMAKE_SOURCE_DIR}/src/interpreter/CopyFragmentizer.h"
        "${CMAKE_SOURCE_DIR}/src/runtime/Runtime.h"
        "${CMAKE_SOURCE_DIR}/src/runtime/HashRuntime.h"
        "${CMAKE_SOURCE_DIR}/src/runtime/HashTableRuntime.h"
        "${CMAKE_SOURCE_DIR}/src/runtime/HashTables.h"
        "${CMAKE_SOURCE_DIR}/src/runtime/MemoryRuntime.h"
        "${CMAKE_SOURCE_DIR}/src/storage/Relation.h"
        )

set(SRC_CC
        "${CMAKE_SOURCE_DIR}/src/algebra/Pipeline.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/Aggregation.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/AggFunctionRegistry.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/CompilationContext.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/Copy.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/RuntimeFunctionSubop.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/ColumnFilter.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/IndexedIUProvider.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/LoopDriver.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/Suboperator.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/aggregation/AggCompute.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/aggregation/AggComputeUnpack.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/aggregation/AggReaderSubop.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/aggregation/AggregatorSubop.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/aggregation/AggState.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/aggregation/AggStateCount.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/expressions/ExpressionHelpers.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/expressions/ExpressionSubop.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/expressions/RuntimeExpressionSubop.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/expressions/RuntimeKeyExpressionSubop.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/row_layout/KeyPackerSubop.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/row_layout/KeyPackingRuntimeState.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/row_layout/KeyUnpackerSubop.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/sinks/CountingSink.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/sinks/FuseChunkSink.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/sources/TableScanSource.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/sources/HashTableSource.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/sources/FuseChunkSource.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/sources/ScratchPadIUProvider.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/TableScan.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/ExpressionOp.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/Filter.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/RelAlgOp.cpp"
        "${CMAKE_SOURCE_DIR}/src/exec/PipelineExecutor.cpp"
        "${CMAKE_SOURCE_DIR}/src/exec/QueryExecutor.cpp"
        "${CMAKE_SOURCE_DIR}/src/exec/runners/PipelineRunner.cpp"
        "${CMAKE_SOURCE_DIR}/src/exec/runners/CompiledRunner.cpp"
        "${CMAKE_SOURCE_DIR}/src/exec/runners/InterpretedRunner.cpp"
        "${CMAKE_SOURCE_DIR}/src/exec/InterruptableJob.cpp"
        "${CMAKE_SOURCE_DIR}/src/storage/Relation.cc"
        "${CMAKE_SOURCE_DIR}/src/codegen/Expression.cpp"
        "${CMAKE_SOURCE_DIR}/src/codegen/IR.cpp"
        "${CMAKE_SOURCE_DIR}/src/codegen/IRBuilder.cpp"
        "${CMAKE_SOURCE_DIR}/src/codegen/Type.cpp"
        "${CMAKE_SOURCE_DIR}/src/codegen/Value.cpp"
        "${CMAKE_SOURCE_DIR}/src/codegen/Statement.cpp"
        "${CMAKE_SOURCE_DIR}/src/codegen/backend_c/BackendC.cpp"
        "${CMAKE_SOURCE_DIR}/src/codegen/backend_c/ScopedWriter.cpp"
        "${CMAKE_SOURCE_DIR}/src/runtime/Runtime.cpp"
        "${CMAKE_SOURCE_DIR}/src/exec/ExecutionContext.cpp"
        "${CMAKE_SOURCE_DIR}/src/exec/FuseChunk.cpp"
        "${CMAKE_SOURCE_DIR}/src/interpreter/AggregationFragmentizer.cpp"
        "${CMAKE_SOURCE_DIR}/src/interpreter/ColumnFilterFragmentizer.cpp"
        "${CMAKE_SOURCE_DIR}/src/interpreter/FragmentCache.cpp"
        "${CMAKE_SOURCE_DIR}/src/interpreter/FragmentGenerator.cpp"
        "${CMAKE_SOURCE_DIR}/src/interpreter/HashTableSourceFragmentizer.cpp"
        "${CMAKE_SOURCE_DIR}/src/interpreter/RuntimeFunctionSubopFragmentizer.cpp"
        "${CMAKE_SOURCE_DIR}/src/interpreter/ExpressionFragmentizer.cpp"
        "${CMAKE_SOURCE_DIR}/src/interpreter/CountingSinkFragmentizer.cpp"
        "${CMAKE_SOURCE_DIR}/src/interpreter/RuntimeExpressionFragmentizer.cpp"
        "${CMAKE_SOURCE_DIR}/src/interpreter/RuntimeKeyExpressionFragmentizer.cpp"
        "${CMAKE_SOURCE_DIR}/src/interpreter/TScanFragmentizer.cpp"
        "${CMAKE_SOURCE_DIR}/src/interpreter/CopyFragmentizer.cpp"
        "${CMAKE_SOURCE_DIR}/src/interpreter/KeyPackingFragmentizer.cpp"
        "${CMAKE_SOURCE_DIR}/src/runtime/HashRuntime.cpp"
        "${CMAKE_SOURCE_DIR}/src/runtime/HashTableRuntime.cpp"
        "${CMAKE_SOURCE_DIR}/src/runtime/HashTables.cpp"
        "${CMAKE_SOURCE_DIR}/src/runtime/MemoryRuntime.cpp"
        )

set(TEST_CC
        "${CMAKE_SOURCE_DIR}/test/test_ir.cpp"
        "${CMAKE_SOURCE_DIR}/test/test_c_backend.cpp"
        "${CMAKE_SOURCE_DIR}/test/test_runtime.cpp"
        "${CMAKE_SOURCE_DIR}/test/algebra/test_repipe.cpp"
        "${CMAKE_SOURCE_DIR}/test/exec/test_interruptable_job.cpp"
        "${CMAKE_SOURCE_DIR}/test/operators/test_aggregation.cpp"
        "${CMAKE_SOURCE_DIR}/test/operators/test_table_scan.cpp"
        "${CMAKE_SOURCE_DIR}/test/operators/test_expression.cpp"
        "${CMAKE_SOURCE_DIR}/test/operators/test_filter.cpp"
        "${CMAKE_SOURCE_DIR}/test/runtime/test_hash_table.cpp"
        "${CMAKE_SOURCE_DIR}/test/suboperators/test_hash_table_source.cpp"
        "${CMAKE_SOURCE_DIR}/test/suboperators/aggregation/test_agg_reader_subop.cpp"
        "${CMAKE_SOURCE_DIR}/test/suboperators/aggregation/test_aggregator_subop.cpp"
        "${CMAKE_SOURCE_DIR}/test/suboperators/test_ht_inserts.cpp"
        "${CMAKE_SOURCE_DIR}/test/suboperators/test_ht_lookup.cpp"
        "${CMAKE_SOURCE_DIR}/test/suboperators/test_key_packing.cpp"
        "${CMAKE_SOURCE_DIR}/test/suboperators/test_scratch_pad_ius.cpp"
        "${CMAKE_SOURCE_DIR}/test/suboperators/test_runtime_param.cpp"
        "${CMAKE_SOURCE_DIR}/test/suboperators/test_minimal_pipeline.cpp"
        "${CMAKE_SOURCE_DIR}/test/suboperators/test_runtime_key_expression.cpp"
        "${CMAKE_SOURCE_DIR}/test/suboperators/test_tscan_source.cpp"
        "${CMAKE_SOURCE_DIR}/test/tpch/test_ingest.cpp"
        "${CMAKE_SOURCE_DIR}/test/test_fragmentizors.cpp"
        )

set(TOOLS_SRC
        "${CMAKE_SOURCE_DIR}/tools/inkfuse_runner.cc"
        )

# ---------------------------------------------------------------------------
# Targets
# ---------------------------------------------------------------------------

# Core inkfuse library, we have to declare it as a shared library
# in order to make runtime symbols visible during dlopen.
add_library(inkfuse STATIC ${SRC_CC})
# We need to expose some of our own symbols within the runtime system, otherwise
# the compiled code will not find it.
# target_link_libraries(inkfuse PUBLIC )
# Need to link to dl in order to open compiled code at runtime
target_link_libraries(inkfuse PRIVATE xxhash_static gflags dl Threads::Threads)


# inkfuse benchmarking binary for running TPC-H queries
add_executable(inkfuse_runner tools/inkfuse_runner.cc)
set_property(TARGET inkfuse_runner PROPERTY ENABLE_EXPORTS 1)
target_link_libraries(inkfuse_runner PRIVATE inkfuse gflags Threads::Threads)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

add_executable(tester test/tester.cc ${TEST_CC})
target_include_directories(tester PRIVATE ${CMAKE_SOURCE_DIR}/test)
target_link_libraries(tester PRIVATE inkfuse gtest gmock Threads::Threads)
enable_testing()
add_test(inkfuse tester)

# ---------------------------------------------------------------------------
# Benchmarks
# ---------------------------------------------------------------------------
option(WITH_BENCH "Build inkfuse with microbenchmark support" OFF)
if (WITH_BENCH)
    # Require gbench system installation for now. Not great, but ok.
    find_package(benchmark REQUIRED)
    add_executable(inkbench bench/benchmarks.cpp bench/ht_benchmark.cpp)
    target_link_libraries(inkbench PUBLIC benchmark::benchmark inkfuse)
endif ()

# ---------------------------------------------------------------------------
# Linting
# ---------------------------------------------------------------------------

option(WITH_LINTING "Enable clang-tidy linting as a taget" OFF)
if (WITH_LINTING)
    include("${CMAKE_SOURCE_DIR}/cmake/clang-tidy.cmake")
    add_clang_tidy_target(src_linting "${SRC_CC}")
    add_clang_tidy_target(include_linting "${INCLUDE_H}")
    add_clang_tidy_target(tools_linting "${TOOLS_SRC}")
    add_clang_tidy_target(test_linting "${TEST_CC}")

    add_custom_target(lint)
    list(APPEND lint_targets include_linting src_linting tools_linting test_linting)
    add_dependencies(lint ${lint_targets})
endif ()

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

message(STATUS "[INKFUSE] settings")
message(STATUS "    GFLAGS_INCLUDE_DIR          = ${GFLAGS_INCLUDE_DIR}")
message(STATUS "    GFLAGS_LIBRARY_PATH         = ${GFLAGS_LIBRARY_PATH}")
message(STATUS "    CMAKE_CXX_FLAGS             = ${CMAKE_CXX_FLAGS}")
message(STATUS "    CMAKE_CXX_FLAGS_DEBUG       = ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "    CMAKE_CXX_FLAGS_RELEASE     = ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "    CMAKE_CXX_FLAGS_RELWITHDEB  = ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
message(STATUS "    WITH_COVERAGE               = ${WITH_COVERAGE}")
message(STATUS "    WITH_LINTING                = ${WITH_LINTING}")
message(STATUS "    JIT_CLANG_11                = ${JIT_CLANG_11}")
message(STATUS "[TEST] settings")
message(STATUS "    XXHASH_INCLUDE_DIR          = ${XXHASH_INCLUDE_DIR}")
message(STATUS "    XXHASH_LIBRARY_PATH         = ${XXHASH_LIBRARY_PATH}")
message(STATUS "    GTEST_INCLUDE_DIR           = ${GTEST_INCLUDE_DIR}")
message(STATUS "    GTEST_LIBRARY_PATH          = ${GTEST_LIBRARY_PATH}")
message(STATUS "    GMOCK_INCLUDE_DIR           = ${GMOCK_INCLUDE_DIR}")
message(STATUS "    GMOCK_LIBRARY_PATH          = ${GMOCK_LIBRARY_PATH}")

