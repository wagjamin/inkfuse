# ---------------------------------------------------------------------------
# IMLAB
# ---------------------------------------------------------------------------

project(inkfuse)
cmake_minimum_required(VERSION 3.7)
# ---------------------------------------------------------------------------
# Environment
# ---------------------------------------------------------------------------

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

find_package(Threads REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)

include("${CMAKE_SOURCE_DIR}/vendor/googletest.cmake")
include("${CMAKE_SOURCE_DIR}/vendor/gflags.cmake")

option(WITH_COVERAGE "Build inkfuse with coverage information attached" OFF)
if (WITH_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage")
endif()

option(JIT_CLANG_11 "Use clang-11 as the default compiler. If off, compiler that should be used is read from CUSTOM_JIT env variable" OFF)
if (JIT_CLANG_11)
    add_compile_definitions(JIT_CLANG_11=${JIT_CLANG_11})
endif()

# ---------------------------------------------------------------------------
# Includes
# ---------------------------------------------------------------------------

include_directories(
    ${CMAKE_SOURCE_DIR}/src
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
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/ExpressionSubop.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/LoopDriver.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/sinks/FuseChunkSink.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/sinks/CountingSink.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/sources/TableScanSource.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/sources/FuseChunkSource.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/Pipeline.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/RelAlgOp.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/ExpressionOp.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/TableScan.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/CompilationContext.h"
        "${CMAKE_SOURCE_DIR}/src/codegen/backend_c/BackendC.h"
        "${CMAKE_SOURCE_DIR}/src/codegen/backend_c/ScopedWriter.h"
        "${CMAKE_SOURCE_DIR}/src/codegen/Backend.h"
        "${CMAKE_SOURCE_DIR}/src/codegen/Expression.h"
        "${CMAKE_SOURCE_DIR}/src/codegen/IR.h"
        "${CMAKE_SOURCE_DIR}/src/codegen/IRBuilder.h"
        "${CMAKE_SOURCE_DIR}/src/codegen/Type.h"
        "${CMAKE_SOURCE_DIR}/src/exec/FuseChunk.h"
        "${CMAKE_SOURCE_DIR}/src/exec/PipelineExecutor.h"
        "${CMAKE_SOURCE_DIR}/src/exec/runners/PipelineRunner.h"
        "${CMAKE_SOURCE_DIR}/src/exec/runners/CompiledRunner.h"
        "${CMAKE_SOURCE_DIR}/src/exec/runners/InterpretedRunner.h"
        "${CMAKE_SOURCE_DIR}/src/exec/ExecutionContext.h"
        "${CMAKE_SOURCE_DIR}/src/exec/InterpretationResult.h"
        "${CMAKE_SOURCE_DIR}/src/interpreter/FragmentCache.h"
        "${CMAKE_SOURCE_DIR}/src/interpreter/FragmentGenerator.h"
        "${CMAKE_SOURCE_DIR}/src/interpreter/ExpressionFragmentizer.h"
        "${CMAKE_SOURCE_DIR}/src/interpreter/CountingSinkFragmentizer.h"
        "${CMAKE_SOURCE_DIR}/src/interpreter/TScanFragmentizer.h"
        "${CMAKE_SOURCE_DIR}/src/interpreter/CopyFragmentizer.h"
        "${CMAKE_SOURCE_DIR}/src/runtime/Runtime.h"
        "${CMAKE_SOURCE_DIR}/src/storage/Relation.h"
        )

set(SRC_CC
        "${CMAKE_SOURCE_DIR}/src/algebra/Pipeline.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/CompilationContext.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/Copy.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/ExpressionSubop.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/IndexedIUProvider.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/LoopDriver.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/Suboperator.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/sinks/CountingSink.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/sinks/FuseChunkSink.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/sources/TableScanSource.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/suboperators/sources/FuseChunkSource.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/TableScan.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/ExpressionOp.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/RelAlgOp.cpp"
        "${CMAKE_SOURCE_DIR}/src/exec/PipelineExecutor.cpp"
        "${CMAKE_SOURCE_DIR}/src/exec/runners/PipelineRunner.cpp"
        "${CMAKE_SOURCE_DIR}/src/exec/runners/CompiledRunner.cpp"
        "${CMAKE_SOURCE_DIR}/src/exec/runners/InterpretedRunner.cpp"
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
        "${CMAKE_SOURCE_DIR}/src/interpreter/FragmentCache.cpp"
        "${CMAKE_SOURCE_DIR}/src/interpreter/FragmentGenerator.cpp"
        "${CMAKE_SOURCE_DIR}/src/interpreter/ExpressionFragmentizer.cpp"
        "${CMAKE_SOURCE_DIR}/src/interpreter/CountingSinkFragmentizer.cpp"
        "${CMAKE_SOURCE_DIR}/src/interpreter/TScanFragmentizer.cpp"
        "${CMAKE_SOURCE_DIR}/src/interpreter/CopyFragmentizer.cpp"
        )

set(TEST_CC
        "${CMAKE_SOURCE_DIR}/test/test_ir.cpp"
        "${CMAKE_SOURCE_DIR}/test/test_c_backend.cpp"
        "${CMAKE_SOURCE_DIR}/test/test_runtime.cpp"
        "${CMAKE_SOURCE_DIR}/test/operators/test_table_scan.cpp"
        "${CMAKE_SOURCE_DIR}/test/operators/test_expression.cpp"
        "${CMAKE_SOURCE_DIR}/test/suboperators/test_minimal_pipeline.cpp"
        "${CMAKE_SOURCE_DIR}/test/suboperators/test_sources.cpp"
        "${CMAKE_SOURCE_DIR}/test/test_fragmentizors.cpp"
        "${CMAKE_SOURCE_DIR}/test/test_repipe.cpp"
        )

set(TOOLS_SRC
    "${CMAKE_SOURCE_DIR}/tools/inkfuse_runner.cc"
        )

# ---------------------------------------------------------------------------
# Targets
# ---------------------------------------------------------------------------

# Core inkfuse library
add_library(inkfuse STATIC ${SRC_CC})
# target_link_libraries(inkfuse PUBLIC )
# Need to link to dl in order to open compiled code at runtime
target_link_libraries(inkfuse PRIVATE gflags dl Threads::Threads)

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
endif()

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
message(STATUS "    GTEST_INCLUDE_DIR           = ${GTEST_INCLUDE_DIR}")
message(STATUS "    GTEST_LIBRARY_PATH          = ${GTEST_LIBRARY_PATH}")
message(STATUS "    GMOCK_INCLUDE_DIR           = ${GMOCK_INCLUDE_DIR}")
message(STATUS "    GMOCK_LIBRARY_PATH          = ${GMOCK_LIBRARY_PATH}")

