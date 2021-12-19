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
#include "query_parse_context.h"
#include "./query_parser.h"

// ISO C++17 does not allow 'register' storage class specifier
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wregister"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wregister"

namespace imlab {
namespace queryc {

// The location of the current token
extern imlab::queryc::location loc;
// The input stream of the scanner
extern std::istream *in;

}  // namespace queryc
}  // namespace imlab

using namespace imlab::queryc;

// Work around an incompatibility in flex (at least versions
// 2.5.31 through 2.5.33): it generates code that does
// not conform to C89.  See Debian bug 333231
// <http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=333231>.
#undef yywrap
#define yywrap() 1

// Declare the yylex function
#define YY_DECL QueryParser::symbol_type yylex(QueryParseContext & qc)
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
";"                 { return QueryParser::make_SEMICOLON(loc); }
","                 { return QueryParser::make_COMMA(loc); }
"("                 { return QueryParser::make_LB(loc); }
")"                 { return QueryParser::make_RB(loc); }
"="                 { return QueryParser::make_EQ(loc); }
"'"                 { return QueryParser::make_QUOTE(loc); }
"select"            { return QueryParser::make_SELECT(loc); }
"from"              { return QueryParser::make_FROM(loc); }
"where"             { return QueryParser::make_WHERE(loc); }
"and"               { return QueryParser::make_AND(loc); }
[a-z][a-z0-9_]*     { return QueryParser::make_IDENTIFIER(yytext, loc); }
[0-9]+              {
                        uint32_t value = 0;
                        try {
                            value = std::stoul(yytext);
                        } catch(const std::invalid_argument &err) {
                            qc.Error(loc.begin.line, loc.begin.column, "integer is invalid");
                        } catch(const std::out_of_range &err) {
                            qc.Error(loc.begin.line, loc.begin.column, "integer out of range");
                        }
                        return QueryParser::make_INTEGER_VALUE(yytext, loc);
                    }
<<EOF>>             { return QueryParser::make_EOF(loc); }
.                   { qc.Error(loc.begin.line, loc.begin.column, "invalid character"); }

%%

// ---------------------------------------------------------------------------------------------------
// Code
// ---------------------------------------------------------------------------------------------------

// The input stream
imlab::queryc::location imlab::queryc::loc;
// The input stream of the scanner
std::istream *imlab::queryc::in = nullptr;

// Begin a scan
void imlab::queryc::QueryParseContext::beginScan(std::istream &is) {
    yy_flex_debug = trace_scanning_;
    in = &is;
}

// End a scan
void imlab::queryc::QueryParseContext::endScan() {
    in = nullptr;
}

