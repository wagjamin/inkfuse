# ---------------------------------------------------------------------------
# IMLAB
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# Bison & Flex
# ---------------------------------------------------------------------------

# Register flex and bison output
set(SCHEMAC_SCANNER_OUT     "${CMAKE_SOURCE_DIR}/tools/schemac/gen/schema_scanner.cc")
set(SCHEMAC_PARSER_OUT      "${CMAKE_SOURCE_DIR}/tools/schemac/gen/schema_parser.cc")
set(SCHEMAC_COMPILER        "${CMAKE_SOURCE_DIR}/tools/schemac/schema_compiler.cc")
set(SCHEMAC_PARSE_CONTEXT   "${CMAKE_SOURCE_DIR}/tools/schemac/schema_parse_context.cc")
set(SCHEMAC_CC ${SCHEMAC_SCANNER_OUT} ${SCHEMAC_PARSER_OUT} ${SCHEMAC_COMPILER} ${SCHEMAC_PARSE_CONTEXT})
set(SCHEMAC_CC_LINTING ${SCHEMAC_COMPILER} ${SCHEMAC_PARSE_CONTEXT})

# Clear the output files
file(WRITE ${SCHEMAC_SCANNER_OUT} "")
file(WRITE ${SCHEMAC_PARSER_OUT} "")

# Generate parser & scanner
add_custom_target(schemac_parser
    COMMAND ${BISON_EXECUTABLE}
        --defines="${CMAKE_SOURCE_DIR}/tools/schemac/gen/schema_parser.h"
        --output=${SCHEMAC_PARSER_OUT}
        --report=state
        --report-file="${CMAKE_BINARY_DIR}/schemac_bison.log"
        "${CMAKE_SOURCE_DIR}/tools/schemac/schema_parser.y"
    COMMAND ${FLEX_EXECUTABLE}
        --outfile=${SCHEMAC_SCANNER_OUT}
        "${CMAKE_SOURCE_DIR}/tools/schemac/schema_scanner.l"
    DEPENDS "${CMAKE_SOURCE_DIR}/tools/schemac/schema_parser.y"
            "${CMAKE_SOURCE_DIR}/tools/schemac/schema_scanner.l")

add_library(schema ${SCHEMAC_CC})
add_dependencies(schema schemac_parser)

# ---------------------------------------------------------------------------
# Compiler
# ---------------------------------------------------------------------------

add_executable(schemac "${CMAKE_SOURCE_DIR}/tools/schemac/schemac.cc")
target_link_libraries(schemac schema gflags Threads::Threads)

# ---------------------------------------------------------------------------
# Tester
# ---------------------------------------------------------------------------

add_executable(tester_schema "${CMAKE_SOURCE_DIR}/tools/schemac/tester.cc")
target_link_libraries(tester_schema schema gflags gtest gmock Threads::Threads)

enable_testing()
add_test(schema tester_schema)

# ---------------------------------------------------------------------------
# Linting
# ---------------------------------------------------------------------------

add_clang_tidy_target(lint_schemac "${SCHEMAC_CC_LINTING}")
list(APPEND lint_targets lint_schemac)

