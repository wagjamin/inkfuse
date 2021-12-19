# ---------------------------------------------------------------------------
# IMLAB
# ---------------------------------------------------------------------------

project(imlab)
cmake_minimum_required(VERSION 3.7)
# ---------------------------------------------------------------------------
# Environment
# ---------------------------------------------------------------------------

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -fsanitize=address")

# Unfortunately, macOS (<= High Sierra) ships with a too old bison version.
# You need to manually install bison and flex via homebrew.
# (It's a good thing to have homebrew installed anyway)
# https://brew.sh/index_de.html
# - brew update
# - brew install bison flex
#
# As homebrew won't override the system's default bison, we need to tell
# cmake where it should look.
if (APPLE)
    list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/bison)
    list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/flex)
endif (APPLE)

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

find_package(Threads REQUIRED)
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
find_package(TBB REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)

include("${CMAKE_SOURCE_DIR}/cmake/clang-tidy.cmake")
include("${CMAKE_SOURCE_DIR}/vendor/googletest.cmake")
include("${CMAKE_SOURCE_DIR}/vendor/gflags.cmake")

include("${CMAKE_SOURCE_DIR}/tools/schemac/local.cmake")
include("${CMAKE_SOURCE_DIR}/tools/queryc/local.cmake")

# ---------------------------------------------------------------------------
# Includes
# ---------------------------------------------------------------------------

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${GTEST_INCLUDE_DIR}
    ${GMOCK_INCLUDE_DIR}
    ${GFLAGS_INCLUDE_DIR}
)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------

set(INCLUDE_H
        "${CMAKE_SOURCE_DIR}/include/gen/tpcc.h"
        "${CMAKE_SOURCE_DIR}/include/imlab/algebra/expression.h"
        "${CMAKE_SOURCE_DIR}/include/imlab/algebra/codegen_helper.h"
    "${CMAKE_SOURCE_DIR}/include/imlab/algebra/inner_join.h"
    "${CMAKE_SOURCE_DIR}/include/imlab/algebra/iu.h"
    "${CMAKE_SOURCE_DIR}/include/imlab/algebra/operator.h"
    "${CMAKE_SOURCE_DIR}/include/imlab/algebra/print.h"
    "${CMAKE_SOURCE_DIR}/include/imlab/algebra/selection.h"
    "${CMAKE_SOURCE_DIR}/include/imlab/algebra/table_scan.h"
    "${CMAKE_SOURCE_DIR}/include/imlab/infra/bits.h"
        "${CMAKE_SOURCE_DIR}/include/imlab/infra/settings.h"
    "${CMAKE_SOURCE_DIR}/include/imlab/infra/error.h"
    "${CMAKE_SOURCE_DIR}/include/imlab/infra/hash.h"
    "${CMAKE_SOURCE_DIR}/include/imlab/infra/hash_table.h"
    "${CMAKE_SOURCE_DIR}/include/imlab/infra/template.h"
    "${CMAKE_SOURCE_DIR}/include/imlab/infra/types.h"
    "${CMAKE_SOURCE_DIR}/include/imlab/storage/relation.h"
    "${CMAKE_SOURCE_DIR}/include/imlab/schemac/schema_compiler.h"
    "${CMAKE_SOURCE_DIR}/include/imlab/schemac/schema_parse_context.h"
    "${CMAKE_SOURCE_DIR}/include/imlab/test/data.h"
    "${CMAKE_SOURCE_DIR}/include/imlab/database.h"
    "${CMAKE_SOURCE_DIR}/include/imlab/schema.h"
        "${CMAKE_SOURCE_DIR}/include/imlab/semana.h"
)

set(SRC_CC
        "${CMAKE_SOURCE_DIR}/src/gen/tpcc.cc"
    "${CMAKE_SOURCE_DIR}/src/database.cc"
        "${CMAKE_SOURCE_DIR}/src/semana.cc"
    "${CMAKE_SOURCE_DIR}/src/infra/types.cc"
        "${CMAKE_SOURCE_DIR}/src/storage/relation.cc"
        "${CMAKE_SOURCE_DIR}/src/storage/relation.cc"
        "${CMAKE_SOURCE_DIR}/src/algebra/codegen_helper.cc"
        "${CMAKE_SOURCE_DIR}/src/algebra/expression.cc"
        "${CMAKE_SOURCE_DIR}/src/algebra/inner_join.cc"
        "${CMAKE_SOURCE_DIR}/src/algebra/iu.cc"
        "${CMAKE_SOURCE_DIR}/src/algebra/print.cc"
        "${CMAKE_SOURCE_DIR}/src/algebra/selection.cc"
        "${CMAKE_SOURCE_DIR}/src/algebra/table_scan.cc"
)

set(TEST_CC
    "${CMAKE_SOURCE_DIR}/test/data.cc"
    "${CMAKE_SOURCE_DIR}/test/database_test.cc"
)

set(TOOLS_SRC
    "${CMAKE_SOURCE_DIR}/tools/imlabdb.cc"
)

# ---------------------------------------------------------------------------
# Targets
# ---------------------------------------------------------------------------

add_library(imlab STATIC ${SRC_CC})
target_link_libraries(imlab PUBLIC queryc_lib TBB::tbb)
target_link_libraries(imlab PRIVATE gflags dl Threads::Threads)

add_executable(imlabdb tools/imlabdb.cc)
set_property(TARGET imlabdb PROPERTY ENABLE_EXPORTS 1)
target_link_libraries(imlabdb PUBLIC imlab queryc_lib)
target_link_libraries(imlabdb PRIVATE gflags Threads::Threads)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

add_executable(tester test/tester.cc ${TEST_CC})
target_link_libraries(tester imlab gtest gmock Threads::Threads)
enable_testing()
add_test(imlab tester)

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

