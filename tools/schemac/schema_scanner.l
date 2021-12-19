%{
// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
%}

%{
// ---------------------------------------------------------------------------------------------------
// Header
// ---------------------------------------------------------------------------------------------------
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <string>
#include <istream>
#include "imlab/schemac/schema_parse_context.h"
#include "./schema_parser.h"

// ISO C++17 does not allow 'register' storage class specifier
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wregister"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wregister"

namespace imlab {
namespace schemac {

// The location of the current token
extern imlab::schemac::location loc;
// The input stream of the scanner
extern std::istream *in;

}  // namespace schemac
}  // namespace imlab

using namespace imlab::schemac;

// Work around an incompatibility in flex (at least versions
// 2.5.31 through 2.5.33): it generates code that does
// not conform to C89.  See Debian bug 333231
// <http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=333231>.
#undef yywrap
#define yywrap() 1

// Declare the yylex function
#define YY_DECL SchemaParser::symbol_type yylex(SchemaParseContext& sc)
// Configure the scanner to use istreams
#define YY_INPUT(buffer, result, max_size)  \
    result = 0; \
    while (true) { \
        int c = in->get(); \
        if (in->eof()) break; \
        buffer[result++] = c; \
        if (result == max_size || c == '\n') break; \
    }
%}

%{
// ---------------------------------------------------------------------------------------------------
// Options
// ---------------------------------------------------------------------------------------------------
%}
%{
// noyywrap:    Disable yywrap (EOF == end of parsing)
// nounput:     Disable manipulation of input stream
// noinput:     Disable explicit fetch of the next character
// batch:       Scanner in batch-mode (vs. interactive)
// debug:       Write debug info to stderr
// caseless:    Case-insensitive pattern matching
%}
%option noyywrap
%option nounput
%option noinput
%option batch 
%option debug
%option caseless

%{
// Code run each time a token is matched.
// We just update the location of the token.
#define YY_USER_ACTION  { loc.columns(yyleng); }
%}

%%

%{
// Code runs each time yylex is called.
// Set the beginning of the token to the end of the previous token.
loc.step ();
%}

[ \t]+              { loc.step(); }
"\n"                { loc.lines (yyleng); loc.step (); }
";"                 { return SchemaParser::make_SEMICOLON(loc); }
","                 { return SchemaParser::make_COMMA(loc); }
"("                 { return SchemaParser::make_LB(loc); }
")"                 { return SchemaParser::make_RB(loc); }
"="                 { return SchemaParser::make_EQ(loc); }
"create"            { return SchemaParser::make_CREATE(loc); }
"table"             { return SchemaParser::make_TABLE(loc); }
"index"             { return SchemaParser::make_INDEX(loc); }
"on"                { return SchemaParser::make_ON(loc); }
"not"               { return SchemaParser::make_NOT(loc); }
"null"              { return SchemaParser::make_NULL(loc); }
"primary"           { return SchemaParser::make_PRIMARY(loc); }
"key"               { return SchemaParser::make_KEY(loc); }
"with"              { return SchemaParser::make_WITH(loc); }
"integer"           { return SchemaParser::make_INTEGER(loc); }
"char"              { return SchemaParser::make_CHAR(loc); }
"varchar"           { return SchemaParser::make_VARCHAR(loc); }
"timestamp"         { return SchemaParser::make_TIMESTAMP(loc); }
"numeric"           { return SchemaParser::make_NUMERIC(loc); }
[a-z][a-z0-9_]*     { return SchemaParser::make_IDENTIFIER(yytext, loc); }
[0-9]+              {
                        uint32_t value = 0;
                        try {
                            value = std::stoul(yytext);
                        } catch(const std::invalid_argument &err) {
                            sc.Error(loc.begin.line, loc.begin.column, "integer is invalid");
                        } catch(const std::out_of_range &err) {
                            sc.Error(loc.begin.line, loc.begin.column, "integer out of range");
                        }
                        return SchemaParser::make_INTEGER_VALUE(value, loc);
                    }
<<EOF>>             { return SchemaParser::make_EOF(loc); }
.                   { sc.Error(loc.begin.line, loc.begin.column, "invalid character"); }

%%

// ---------------------------------------------------------------------------------------------------
// Code
// ---------------------------------------------------------------------------------------------------

// The input stream
imlab::schemac::location imlab::schemac::loc;
// The input stream of the scanner
std::istream *imlab::schemac::in = nullptr;

// Begin a scan
void imlab::schemac::SchemaParseContext::beginScan(std::istream &is) {
    yy_flex_debug = trace_scanning_;
    in = &is;
}

// End a scan
void imlab::schemac::SchemaParseContext::endScan() {
    in = nullptr;
}

