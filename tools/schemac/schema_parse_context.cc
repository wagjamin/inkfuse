// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#include "imlab/schemac/schema_parse_context.h"
#include "./gen/schema_parser.h"
#include <sstream>
#include <unordered_set>
#include "imlab/infra/error.h"
// ---------------------------------------------------------------------------------------------------
using SchemaParseContext = imlab::schemac::SchemaParseContext;
using Table = imlab::schemac::Table;
using Schema = imlab::schemac::Schema;
using Type = imlab::schemac::Type;
// ---------------------------------------------------------------------------------------------------
thread_local imlab::schemac::Schema bison_parse_result;
// ---------------------------------------------------------------------------------------------------
// Constructor
SchemaParseContext::SchemaParseContext(bool trace_scanning, bool trace_parsing)
    : trace_scanning_(trace_scanning), trace_parsing_(trace_parsing) {}
// ---------------------------------------------------------------------------------------------------
// Destructor
SchemaParseContext::~SchemaParseContext() {}
// ---------------------------------------------------------------------------------------------------
// Parse a string
Schema SchemaParseContext::Parse(std::istream &in) {
    beginScan(in);
    imlab::schemac::SchemaParser parser(*this);
    parser.set_debug_level(trace_parsing_);
    parser.parse();
    endScan();

    auto old = std::move(bison_parse_result);
    bison_parse_result = Schema {};

    // Do semantic verification enforcing that the schema definition was valid.
    validate(old);

    return old;
}
// ---------------------------------------------------------------------------------------------------
void SchemaParseContext::validate(const Schema &schema)
{
    for (const auto& table: schema.tables) {
        std::unordered_set<std::string_view> colnames;
        for (const auto& column: table.columns) {
            if (colnames.count(column.id)) {
                throw std::runtime_error("Column " + column.id + " appears twice");
            }
            colnames.insert(column.id);
        }
        for (const auto& pk_col: table.primary_key) {
            if (!colnames.count(pk_col)) {
                throw std::runtime_error("Invalid PK column " + pk_col + " - does not exist");
            }
        }
    }
    // TODO(Benjamin) Add index verification.
}
// ---------------------------------------------------------------------------------------------------
// Yield an error
void SchemaParseContext::Error(const std::string& m) {
    throw SchemaCompilationError(m);
}
// ---------------------------------------------------------------------------------------------------
// Yield an error
void SchemaParseContext::Error(uint32_t line, uint32_t column, const std::string &err) {
    std::stringstream ss;
    ss << "[ l=" << line << " c=" << column << " ] " << err << std::endl;
    throw SchemaCompilationError(ss.str());
}
// ---------------------------------------------------------------------------------------------------

