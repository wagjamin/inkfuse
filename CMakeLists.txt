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
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -fsanitize=address")

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

find_package(Threads REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)

include("${CMAKE_SOURCE_DIR}/cmake/clang-tidy.cmake")
include("${CMAKE_SOURCE_DIR}/vendor/googletest.cmake")
include("${CMAKE_SOURCE_DIR}/vendor/gflags.cmake")

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
        "${CMAKE_SOURCE_DIR}/src/algebra/Operator.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/Pipeline.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/ColumnScan.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/CompilationContext.h"
        "${CMAKE_SOURCE_DIR}/src/algebra/FuseChunkSink.h"
        "${CMAKE_SOURCE_DIR}/src/codegen/backend_c/BackendC.h"
        "${CMAKE_SOURCE_DIR}/src/codegen/backend_c/ScopedWriter.h"
        "${CMAKE_SOURCE_DIR}/src/codegen/Backend.h"
        "${CMAKE_SOURCE_DIR}/src/codegen/Expression.h"
        "${CMAKE_SOURCE_DIR}/src/codegen/IR.h"
        "${CMAKE_SOURCE_DIR}/src/codegen/IRBuilder.h"
        "${CMAKE_SOURCE_DIR}/src/codegen/Type.h"
        "${CMAKE_SOURCE_DIR}/src/exec/FuseChunk.h"
        "${CMAKE_SOURCE_DIR}/src/exec/InterpretationResult.h"
        "${CMAKE_SOURCE_DIR}/src/infra/types.h"
        "${CMAKE_SOURCE_DIR}/src/interpreter/FragmentCache.h"
        "${CMAKE_SOURCE_DIR}/src/interpreter/FragmentGenerator.h"
        "${CMAKE_SOURCE_DIR}/src/interpreter/FragmentGenerator.h"
        "${CMAKE_SOURCE_DIR}/src/runtime/QueryExecutor.h"
        "${CMAKE_SOURCE_DIR}/src/storage/Relation.h"
        )

set(SRC_CC
        "${CMAKE_SOURCE_DIR}/src/algebra/ColumnScan.cpp"
        "${CMAKE_SOURCE_DIR}/src/algebra/FuseChunkSink.cpp"
        "${CMAKE_SOURCE_DIR}/src/infra/types.cc"
        "${CMAKE_SOURCE_DIR}/src/storage/Relation.cc"
        "${CMAKE_SOURCE_DIR}/src/codegen/Expression.cpp"
        "${CMAKE_SOURCE_DIR}/src/codegen/IR.cpp"
        "${CMAKE_SOURCE_DIR}/src/codegen/IRBuilder.cpp"
        "${CMAKE_SOURCE_DIR}/src/codegen/Type.cpp"
        "${CMAKE_SOURCE_DIR}/src/codegen/Value.cpp"
        "${CMAKE_SOURCE_DIR}/src/codegen/Statement.cpp"
        "${CMAKE_SOURCE_DIR}/src/codegen/backend_c/BackendC.cpp"
        "${CMAKE_SOURCE_DIR}/src/codegen/backend_c/ScopedWriter.cpp"
        "${CMAKE_SOURCE_DIR}/src/exec/FuseChunk.cpp"
        )

set(TEST_CC
        "${CMAKE_SOURCE_DIR}/test/test_ir.cpp"
        "${CMAKE_SOURCE_DIR}/test/test_c_backend.cpp"
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
target_link_libraries(inkfuse_runner PUBLIC inkfuse)
target_link_libraries(inkfuse_runner PRIVATE gflags Threads::Threads)

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

add_clang_tidy_target(src_linting "${SRC_CC}")
add_clang_tidy_target(include_linting "${INCLUDE_H}")
add_clang_tidy_target(tools_linting "${TOOLS_SRC}")
add_clang_tidy_target(test_linting "${TEST_CC}")

add_custom_target(lint)
list(APPEND lint_targets include_linting src_linting tools_linting test_linting)
add_dependencies(lint ${lint_targets})

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

message(STATUS "[IMLAB] settings")
message(STATUS "    GFLAGS_INCLUDE_DIR          = ${GFLAGS_INCLUDE_DIR}")
message(STATUS "    GFLAGS_LIBRARY_PATH         = ${GFLAGS_LIBRARY_PATH}")
message(STATUS "[TEST] settings")
message(STATUS "    GTEST_INCLUDE_DIR           = ${GTEST_INCLUDE_DIR}")
message(STATUS "    GTEST_LIBRARY_PATH          = ${GTEST_LIBRARY_PATH}")
message(STATUS "    GMOCK_INCLUDE_DIR           = ${GMOCK_INCLUDE_DIR}")
message(STATUS "    GMOCK_LIBRARY_PATH          = ${GMOCK_LIBRARY_PATH}")

