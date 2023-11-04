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
# Generate DWARF 4 in debug to work on older GDB versions
# flto required as xxhash is also built with flto to allow efficient inlining
# of the hash functions.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -gdwarf-4 -flto")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g -O3")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

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

option(JIT_CLANG_14 "Use clang-14 as the default compiler. If off, compiler that should be used is read from CUSTOM_JIT env variable" ON)
if (JIT_CLANG_14)
    add_compile_definitions(WITH_JIT_CLANG_14)
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

# Runtime System C++ Files - directly linked against the generated code.
set(RUNTIME_SRC_CC
        "${CMAKE_SOURCE_DIR}/src/runtime/ExternRuntime.cpp"
        "${CMAKE_SOURCE_DIR}/src/runtime/HashTables.cpp"
        "${CMAKE_SOURCE_DIR}/src/runtime/NewHashTables.cpp"
        "${CMAKE_SOURCE_DIR}/src/runtime/MemoryRuntime.cpp"
        "${CMAKE_SOURCE_DIR}/src/runtime/TupleMaterializer.cpp"
    )

# Inkfuse C++ Files - the actual database system: executors, code generation logic, ...
set(SRC_CC
        "${CMAKE_SOURCE_DIR}/src/algebra/Pipeline.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/Print.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/Aggregation.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/AggregationMerger.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/AggFunctionRegistry.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/CompilationContext.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/Copy.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/RuntimeFunctionSubop.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/ColumnFilter.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/IndexedIUProvider.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/LoopDriver.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/Suboperator.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/aggregation/AggCompute.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/aggregation/AggComputeAvg.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/aggregation/AggComputeUnpack.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/aggregation/AggReaderSubop.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/aggregation/AggregatorSubop.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/aggregation/AggState.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/aggregation/AggStateCount.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/aggregation/AggStateSum.cpp"
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
        "${CMAKE_SOURCE_DIR}/src/algebra/Join.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/RelAlgOp.cpp"
        "${CMAKE_SOURCE_DIR}/src/exec/PipelineExecutor.cpp"
        "${CMAKE_SOURCE_DIR}/src/exec/QueryExecutor.cpp"
        "${CMAKE_SOURCE_DIR}/src/exec/runners/PipelineRunner.cpp"
        "${CMAKE_SOURCE_DIR}/src/exec/runners/CompiledRunner.cpp"
        "${CMAKE_SOURCE_DIR}/src/exec/runners/InterpretedRunner.cpp"
        "${CMAKE_SOURCE_DIR}/src/exec/InterruptableJob.cpp"
        "${CMAKE_SOURCE_DIR}/src/storage/Relation.cpp"
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
        "${CMAKE_SOURCE_DIR}/src/exec/DeferredState.cpp"
        "${CMAKE_SOURCE_DIR}/src/common/Barrier.cpp"
        "${CMAKE_SOURCE_DIR}/src/common/Helpers.cpp"
        "${CMAKE_SOURCE_DIR}/src/common/TPCH.cpp"
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
        "${CMAKE_SOURCE_DIR}/src/runtime/HashTableRuntime.cpp"
        )

set(TEST_CC
        "${CMAKE_SOURCE_DIR}/test/test_ir.cpp"
        "${CMAKE_SOURCE_DIR}/test/test_barrier.cpp"
        "${CMAKE_SOURCE_DIR}/test/test_c_backend.cpp"
        "${CMAKE_SOURCE_DIR}/test/test_runtime.cpp"
        "${CMAKE_SOURCE_DIR}/test/algebra/test_repipe.cpp"
        "${CMAKE_SOURCE_DIR}/test/exec/test_interruptable_job.cpp"
        "${CMAKE_SOURCE_DIR}/test/multithreading/test_aggregation.cpp"
        "${CMAKE_SOURCE_DIR}/test/multithreading/test_join.cpp"
        "${CMAKE_SOURCE_DIR}/test/multithreading/test_scan_expr_filter.cpp"
        "${CMAKE_SOURCE_DIR}/test/operators/test_aggregation.cpp"
        "${CMAKE_SOURCE_DIR}/test/operators/test_table_scan.cpp"
        "${CMAKE_SOURCE_DIR}/test/operators/test_expression.cpp"
        "${CMAKE_SOURCE_DIR}/test/operators/test_filter.cpp"
        "${CMAKE_SOURCE_DIR}/test/operators/test_join.cpp"
        "${CMAKE_SOURCE_DIR}/test/runtime/test_hash_table.cpp"
        "${CMAKE_SOURCE_DIR}/test/runtime/test_atomic_hash_table.cpp"
        "${CMAKE_SOURCE_DIR}/test/runtime/test_atomic_hash_table_complex_key.cpp"
        "${CMAKE_SOURCE_DIR}/test/runtime/test_hash_table_complex_key.cpp"
        "${CMAKE_SOURCE_DIR}/test/runtime/test_tuple_materializer.cpp"
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
        "${CMAKE_SOURCE_DIR}/test/test_storage.cpp"
        "${CMAKE_SOURCE_DIR}/test/tpch/test_queries.cpp"
        )

set(TOOLS_SRC
        "${CMAKE_SOURCE_DIR}/tools/inkfuse_bench.cpp"
        "${CMAKE_SOURCE_DIR}/tools/inkfuse_runner.cpp"
        )

# ---------------------------------------------------------------------------
# Targets
# ---------------------------------------------------------------------------
# The inkfuse runtime: exposes functions that will be called from the generated
# code. The purpose of the runtime is to be able to write C++ code for e.g.
# hash tables, and make it easy to export these symbols to the generated
# C code.

# Object library that we use in other targets to expose runtime functions.
add_library(inkfuse_runtime OBJECT ${RUNTIME_SRC_CC})

# Static library for the inkfuse runtime. This is actually where some of the real
# magic happens: we generate the `inkfuse_runtime_static` static library.
# We generate this with `flto` and `fPIC`, which generates position independent
# LLVM Bytecode in the end.
# We then install that static library in tmp and unpack it into object
# files again. When generating C code in the runtime, we statically link against
# these object files before generating the shared query object.
# This allows us to inline runtime C++ code into the generated C code.
add_library(inkfuse_runtime_static STATIC ${RUNTIME_SRC_CC})
target_compile_options(inkfuse_runtime_static PRIVATE -flto -fPIC)

# Install 
add_custom_target(inkfuse_install_runtime
    DEPENDS inkfuse_runtime_static xxhash_static
    COMMAND 
        cp libinkfuse_runtime_static.a /tmp
    # Generate XXHash LLVM IR code
    COMMAND 
        clang-14 -O3 -fPIC -flto vendor/xxhash/src/xxhash_src/xxhash.c -c -o xxhash_static.o
    COMMAND 
        cp xxhash_static.o /tmp
    # Unpack the runtime objects
    COMMAND 
        ar --output /tmp x libinkfuse_runtime_static.a
    )

# Add the output object files that we need to do link time optimization against
# when generating code for the interpreter.
ADD_DEFINITIONS( "-D_INKFUSE_OBJECT_DEPENDENCIES=\" \
      /tmp/xxhash_static.o  \
      /tmp/ExternRuntime.cpp.o  \
      /tmp/HashTables.cpp.o  \
      /tmp/MemoryRuntime.cpp.o  \
      /tmp/NewHashTables.cpp.o \
      /tmp/TupleMaterializer.cpp.o \
      \"")

# Core inkfuse library, we have to declare it as a shared library
# in order to make runtime symbols visible during dlopen.
add_library(inkfuse STATIC ${SRC_CC})
# We need to expose some of our own symbols within the runtime system, otherwise
# the compiled code will not find it.
# target_link_libraries(inkfuse PUBLIC )
# Need to link to dl in order to open compiled code at runtime
target_link_libraries(inkfuse PRIVATE xxhash_static dl Threads::Threads)
# Every time we depend on inkfuse in some way we need to rebuild the runtime
add_dependencies(inkfuse inkfuse_install_runtime)

# inkfuse binary for running TPC-H queries in an interactive way
add_executable(inkfuse_runner 
    tools/inkfuse_runner.cpp 
    $<TARGET_OBJECTS:inkfuse_runtime>
)
set_property(TARGET inkfuse_runner PROPERTY ENABLE_EXPORTS 1)
target_link_libraries(inkfuse_runner PRIVATE inkfuse gflags Threads::Threads)

# inkfuse binary for benchmarking TPC-H queries in the reproducibility folder
add_executable(inkfuse_bench 
    tools/inkfuse_bench.cpp
    $<TARGET_OBJECTS:inkfuse_runtime>
)
set_property(TARGET inkfuse_bench PROPERTY ENABLE_EXPORTS 1)
target_link_libraries(inkfuse_bench PRIVATE inkfuse gflags Threads::Threads)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

add_executable(tester 
    test/tester.cc 
    ${TEST_CC}
    $<TARGET_OBJECTS:inkfuse_runtime>
)
set_property(TARGET tester PROPERTY ENABLE_EXPORTS 1)
target_include_directories(tester PRIVATE ${CMAKE_SOURCE_DIR}/test)
target_link_libraries(tester PRIVATE inkfuse gtest gmock Threads::Threads)
# Move the testdata into the binary tree for easy ingest tests.
file(COPY test/tpch/testdata DESTINATION test/tpch)
enable_testing()
add_test(inkfuse tester)

# ---------------------------------------------------------------------------
# Benchmarks
# ---------------------------------------------------------------------------
option(WITH_BENCH "Build inkfuse with microbenchmark support" OFF)
if (WITH_BENCH)
    # Require gbench system installation for now. Not great, but ok.
    find_package(benchmark REQUIRED)
    add_executable(inkbench bench/benchmarks.cpp 
        bench/compiler_invoke.cpp 
        bench/ht_benchmark.cpp 
        bench/vectorized_ht.cpp
        )
    target_link_libraries(inkbench PUBLIC benchmark::benchmark inkfuse)
    # Move the testdata into the binary tree for easy ingest tests.
    file(COPY bench/testdata DESTINATION bench)
endif ()

# ---------------------------------------------------------------------------
# Linting
# ---------------------------------------------------------------------------

option(WITH_LINTING "Enable clang-tidy linting as a taget" OFF)
if (WITH_LINTING)
    include("${CMAKE_SOURCE_DIR}/cmake/clang-tidy.cmake")
    add_clang_tidy_target(src_linting "${SRC_CC}")
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
message(STATUS "    JIT_CLANG_14                = ${JIT_CLANG_14}")
message(STATUS "[TEST] settings")
message(STATUS "    XXHASH_INCLUDE_DIR          = ${XXHASH_INCLUDE_DIR}")
message(STATUS "    XXHASH_LIBRARY_PATH         = ${XXHASH_LIBRARY_PATH}")
message(STATUS "    GTEST_INCLUDE_DIR           = ${GTEST_INCLUDE_DIR}")
message(STATUS "    GTEST_LIBRARY_PATH          = ${GTEST_LIBRARY_PATH}")
message(STATUS "    GMOCK_INCLUDE_DIR           = ${GMOCK_INCLUDE_DIR}")
message(STATUS "    GMOCK_LIBRARY_PATH          = ${GMOCK_LIBRARY_PATH}")

