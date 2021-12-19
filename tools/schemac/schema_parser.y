// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
%skeleton "lalr1.cc"
%require "3.0.4"
// ---------------------------------------------------------------------------------------------------
// Write a parser header file
%defines
// Define the parser class name
%define api.parser.class {SchemaParser}
// Create the parser in our namespace
%define api.namespace { imlab::schemac }
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
%param { imlab::schemac::SchemaParseContext &sc }
// ---------------------------------------------------------------------------------------------------
// Added to the header file and parser implementation before bison definitions.
// We include string for string tokens and forward declare the SchemaParseContext.
%code requires {
#include <string>
#include <vector>
#include "imlab/schemac/schema_parse_context.h"

extern thread_local imlab::schemac::Schema bison_parse_result;
}
// ---------------------------------------------------------------------------------------------------
// Import the compiler header in the implementation file
%code {
imlab::schemac::SchemaParser::symbol_type yylex(imlab::schemac::SchemaParseContext& sc);
}
// ---------------------------------------------------------------------------------------------------
// Token definitions
%token <int>            INTEGER_VALUE    "integer_value"
%token <std::string>    IDENTIFIER       "identifier"
%token LB               "left_brackets"
%token RB               "right_brackets"
%token EQ               "equals"
%token SEMICOLON        "semicolon"
%token INTEGER          "integer"
%token CHAR             "char"
%token VARCHAR          "varchar"
%token TIMESTAMP        "timestamp"
%token NUMERIC          "numeric"
%token COMMA            "comma"
%token CREATE           "create"
%token TABLE            "table"
%token INDEX            "index"
%token ON               "on"
%token NOT              "not"
%token NULL             "null"
%token PRIMARY          "primary"
%token KEY              "key"
%token WITH             "with"
%token EOF 0            "eof"
// ---------------------------------------------------------------------------------------------------
%type <std::vector<imlab::schemac::Column>> col_list;
%type <std::pair<std::vector<imlab::schemac::Column>, std::vector<std::string>>> opt_column_desc;
%type <imlab::schemac::Column> col_def;
%type <std::vector<std::string>> pk_opt col_name_list;
%type <imlab::schemac::Type> type;
%type <imlab::schemac::IndexType> key_index_type_opt;
// ---------------------------------------------------------------------------------------------------
%%

%start sql_statement_list_opt;

sql_statement_list_opt:
    sql_statement_list
 |  %empty
    ;

sql_statement_list:
    sql_statement_list sql_statement
 |  sql_statement
    ;

sql_statement:
    create_table_statement
 |  create_index_statement
    ;

create_table_statement:
    CREATE TABLE IDENTIFIER LB opt_column_desc RB key_index_type_opt SEMICOLON
    {
        auto& [col_desc, pk_desc] = $5;
        Table table {
            .id = std::move($3),
            .columns = std::move(col_desc),
            .primary_key = std::move(pk_desc),
            .index_type = std::move($7),
        };
        bison_parse_result.tables.push_back(std::move(table));
    }
    ;

opt_column_desc:
    col_list pk_opt { $$ = {std::move($1), std::move($2)};};
    | %empty { $$ = {{}, {}}; }
    ;


col_list:
    col_list COMMA col_def
    {
        $1.push_back(std::move($3));
        $$ = std::move($1);
    }
 |  col_def  { $$ = std::vector<imlab::schemac::Column>{std::move($1)}; }
    ;

col_def:
    IDENTIFIER type NOT NULL
    {
        $$ = Column {
            .id = std::move($1),
            .type = std::move($2),
        };
    }
    ;

type:
    INTEGER                                           { $$ = imlab::schemac::Type::Integer(); }
 |  TIMESTAMP                                         { $$ = imlab::schemac::Type::Timestamp(); }
 |  CHAR LB INTEGER_VALUE RB                          { $$ = imlab::schemac::Type::Char($3); }
 |  VARCHAR LB INTEGER_VALUE RB                       { $$ = imlab::schemac::Type::Varchar($3); }
 |  NUMERIC LB INTEGER_VALUE COMMA INTEGER_VALUE RB   { $$ = imlab::schemac::Type::Numeric($3, $5); }
    ;

pk_opt:
   COMMA PRIMARY KEY LB col_name_list RB { $$ = std::move($5); };
 | %empty { $$ = std::vector<std::string>{}; };
    ;

col_name_list:
   col_name_list COMMA IDENTIFIER
   {
        $1.push_back(std::move($3));
        $$ = std::move($1);
   }
 | IDENTIFIER { $$ = std::vector<std::string>{std::move($1)}; }
   ;

key_index_type_opt:
    WITH LB IDENTIFIER EQ IDENTIFIER RB
    {
        // TODO(Benjamin) dirty hack, turn this into a proper settings parser
        if ($3 != "key_index_type" && $3 != "index_type") {
            error(@3, "Invalid setting " + $3);
        }
        if ($5 == "unordered_map") {
            $$ = imlab::schemac::kSTLUnorderedMap;
        } else if ($5 == "ordered_map") {
            $$ = imlab::schemac::kSTLMap;
        } else if ($5 == "btree_map") {
            $$ = imlab::schemac::kSTXMap;
        } else {
            error(@3, "Invalid index type " + $5);
        }
    }
   | %empty { $$ = imlab::schemac::kSTLUnorderedMap; }
   ;

create_index_statement:
    CREATE INDEX IDENTIFIER ON IDENTIFIER LB col_name_list RB key_index_type_opt SEMICOLON
    {
        Index index{
            .id = std::move($3),
            .table_id = std::move($5),
            .columns = std::move($7),
            .index_type = std::move($9),
        };
        bison_parse_result.indexes.push_back(std::move(index));
    };

%%
// ---------------------------------------------------------------------------------------------------
// Define error function
void imlab::schemac::SchemaParser::error(const location_type& l, const std::string& m) {
    sc.Error(l.begin.line, l.begin.column, m);
}
// ---------------------------------------------------------------------------------------------------

