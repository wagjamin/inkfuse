# ---------------------------------------------------------------------------
# IMLAB
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# Query Parsing Lib
# ---------------------------------------------------------------------------

set(QUERYC_SCANNER_OUT     "${CMAKE_SOURCE_DIR}/tools/queryc/gen/query_scanner.cc")
set(QUERYC_PARSER_OUT      "${CMAKE_SOURCE_DIR}/tools/queryc/gen/query_parser.cc")
# set(QUERYC_COMPILER        "${CMAKE_SOURCE_DIR}/tools/queryc/schema_compiler.cc")
set(QUERYC_PARSE_CONTEXT   "${CMAKE_SOURCE_DIR}/tools/queryc/query_parse_context.cc")
set(QUERYC_CODE_GENERATOR  "${CMAKE_SOURCE_DIR}/tools/queryc/code_generator.cc")
set(QUERYC_CC ${QUERYC_SCANNER_OUT} ${QUERYC_PARSER_OUT} ${QUERYC_COMPILER} ${QUERYC_PARSE_CONTEXT} ${QUERYC_CODE_GENERATOR})
set(QUERYC_CC_LINTING ${QUERYC_COMPILER} ${QUERYC_PARSE_CONTEXT})

# Clear the output files
file(WRITE ${QUERYC_SCANNER_OUT} "")
file(WRITE ${QUERYC_PARSER_OUT} "")

# Generate parser & scanner
add_custom_target(queryc_parser
        COMMAND ${BISON_EXECUTABLE}
        --defines="${CMAKE_SOURCE_DIR}/tools/queryc/gen/query_parser.h"
        --output=${QUERYC_PARSER_OUT}
        --report=state
        --report-file="${CMAKE_BINARY_DIR}/queryc_bison.log"
        "${CMAKE_SOURCE_DIR}/tools/queryc/query_parser.y"
        COMMAND ${FLEX_EXECUTABLE}
        --outfile=${QUERYC_SCANNER_OUT}
        "${CMAKE_SOURCE_DIR}/tools/queryc/query_scanner.l"
        DEPENDS "${CMAKE_SOURCE_DIR}/tools/queryc/query_parser.y"
        "${CMAKE_SOURCE_DIR}/tools/queryc/query_scanner.l")


add_library(queryc_lib ${QUERYC_CC})
add_dependencies(queryc_lib queryc_parser)

target_include_directories(queryc_lib PUBLIC ${CMAKE_SOURCE_DIR}/tools/queryc)

# ---------------------------------------------------------------------------
# Compiler
# ---------------------------------------------------------------------------

add_executable(queryc "${CMAKE_SOURCE_DIR}/tools/queryc/queryc.cc" ${INCLUDE_H})
target_link_libraries(queryc imlab gflags Threads::Threads)

# ---------------------------------------------------------------------------
add_executable(tester_queryc "${CMAKE_SOURCE_DIR}/tools/queryc/tester.cc")
target_link_libraries(tester_queryc queryc_lib gflags gtest gmock Threads::Threads)

enable_testing()
add_test(queryc tester_queryc)
# ---------------------------------------------------------------------------
# Linting
# ---------------------------------------------------------------------------

add_clang_tidy_target(lint_queryc "${CMAKE_SOURCE_DIR}/tools/queryc/queryc.cc")
list(APPEND lint_targets lint_queryc)

