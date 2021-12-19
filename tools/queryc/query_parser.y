// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
%skeleton "lalr1.cc"
%require "3.0.4"
// ---------------------------------------------------------------------------------------------------
// Write a parser header file
%defines
// Define the parser class name
%define api.parser.class {QueryParser}
// Create the parser in our namespace
%define api.namespace { imlab::queryc }
// Use C++ variant to store the values and get better type warnings (compared to "union")
%define api.value.type variant
// With variant-based values, symbols are handled as a whole in the scanner
%define api.token.constructor
// Prefix all tokens
%define api.token.prefix {SCHEMA_}
// Check if variants are constructed and destroyed properly
%define parse.assert
// Trace the parser
%define parse.trace
// Use verbose parser errors
%define parse.error verbose
// Enable location tracking.
%locations
// Pass the compiler as parameter to yylex/yyparse.
%param { imlab::queryc::QueryParseContext &qc }
// ---------------------------------------------------------------------------------------------------
// Added to the header file and parser implementation before bison definitions.
// We include string for string tokens and forward declare the SchemaParseContext.
%code requires {
#include <string>
#include <vector>
#include <tuple>
#include "query_ast.h"
#include "query_parse_context.h"

extern imlab::queryc::QueryAST bison_query_parse_result;
}
// ---------------------------------------------------------------------------------------------------
// Import the compiler header in the implementation file
%code {
imlab::queryc::QueryParser::symbol_type yylex(imlab::queryc::QueryParseContext& qc);
}
// ---------------------------------------------------------------------------------------------------
// Token definitions
%token <std::string>    INTEGER_VALUE    "integer_value"
%token <std::string>    IDENTIFIER       "identifier"
%token LB               "left_brackets"
%token RB               "right_brackets"
%token EQ               "equals"
%token QUOTE            "quote"
%token AND              "and"
%token SEMICOLON        "semicolon"
%token COMMA            "comma"
%token SELECT           "select"
%token FROM             "from"
%token WHERE            "where"
%token EOF 0            "eof"
// ---------------------------------------------------------------------------------------------------
%type <std::vector<std::string>> non_empty_identifier_list identifier_list;
%type <std::vector<std::tuple<std::string, std::string, PredType>>> non_empty_where_list optional_where_list where_list;
%type <std::tuple<std::string, std::string, PredType>> where_clause;
%type <std::pair<std::string, PredType>> pred_clause;
// ---------------------------------------------------------------------------------------------------
%%

%start sql_query;

sql_query:
    SELECT non_empty_identifier_list FROM non_empty_identifier_list optional_where_list SEMICOLON
    {
        bison_query_parse_result.select_list = std::move($2);
        bison_query_parse_result.from_list = std::move($4);
        bison_query_parse_result.where_list = std::move($5);
    }
    ;

non_empty_identifier_list:
    IDENTIFIER
    {
        $$ = {std::move($1)};
    }
 |  identifier_list IDENTIFIER
    {
        $1.push_back(std::move($2));
        $$ = std::move($1);
    }
    ;

identifier_list:
    identifier_list IDENTIFIER COMMA
    {
        $1.push_back(std::move($2));
        $$ = std::move($1);
    }
 |  IDENTIFIER COMMA { $$ = {std::move($1)}; }
    ;

optional_where_list:
    WHERE non_empty_where_list { $$ = std::move($2); }
 |  %empty { $$ = {}; }
    ;

non_empty_where_list:
    where_clause { $$ = {std::move($1)}; }
 |  where_list where_clause
    {
        $1.push_back(std::move($2));
        $$ = std::move($1);
    }
    ;

where_list:
    where_list where_clause AND
    {
        $1.push_back(std::move($2));
        $$ = std::move($1);
    }
 |  where_clause AND { $$ = {std::move($1)}; };
    ;

where_clause:
    IDENTIFIER EQ pred_clause
    {
        std::get<0>($$) = std::move($1);
        std::get<1>($$) = $3.first;
        std::get<2>($$) = $3.second;
    }
    ;

pred_clause:
    IDENTIFIER { $$ = {$1, PredType::ColumnRef}; }
 |  INTEGER_VALUE { $$ = {$1, PredType::IntConstant}; }
 |  QUOTE IDENTIFIER QUOTE { $$ = {$2, PredType::StringConstant}; }
    ;

%%
// ---------------------------------------------------------------------------------------------------
// Define error function
void imlab::queryc::QueryParser::error(const location_type& l, const std::string& m) {
    qc.Error(l.begin.line, l.begin.column, m);
}
// ---------------------------------------------------------------------------------------------------

