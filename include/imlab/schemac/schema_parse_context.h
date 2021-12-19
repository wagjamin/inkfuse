// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_SCHEMAC_SCHEMA_PARSE_CONTEXT_H_
#define INCLUDE_IMLAB_SCHEMAC_SCHEMA_PARSE_CONTEXT_H_
// ---------------------------------------------------------------------------------------------------
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <stdexcept>
// ---------------------------------------------------------------------------------------------------
namespace imlab {
namespace schemac {
// ---------------------------------------------------------------------------------------------------
struct SchemaParser;
struct SchemaCompiler;
// ---------------------------------------------------------------------------------------------------
// A single type
// Required by test/schemac/schema_parser_test.cc
struct Type {
    // Type class
    enum Class: uint8_t {
        kInteger,
        kTimestamp,
        kNumeric,
        kChar,
        kVarchar,
        kBool,
    };
    // The type class
    Class tclass;
    // The type argument (if any)
    union {
        struct {
            uint32_t length;
            uint32_t precision;
        };
    };

    // Get type name
    const char *Name() const {
        switch (tclass) {
            case kInteger:      return "Integer";
            case kTimestamp:    return "Timestamp";
            case kNumeric:      return "Numeric";
            case kChar:         return "Character";
            case kVarchar:      return "Varchar";
            case kBool:         return "Bool";
            default:            return "Unknown";
        }
    };

    // Get full type description
    std::string Desc() const {
        switch (tclass) {
            case kInteger:      return "Integer";
            case kTimestamp:    return "Timestamp";
            case kNumeric:      return "Numeric<" + std::to_string(length) + "," + std::to_string(precision) + ">";
            case kChar:         return "Char<" + std::to_string(length)+ ">";
            case kVarchar:      return "Varchar<" + std::to_string(length) + ">";
            case kBool:         return "bool";
            default:            return "Unknown";
        }
    };

    bool operator==(const Type& other) const {
        return tclass == other.tclass && (tclass != kNumeric || (length == other.length && precision == other.precision));
    }

    // Static methods to construct a column
    static Type Integer() {
        Type t; t.tclass = kInteger; return t;
    };
    static Type Timestamp() {
        Type t; t.tclass = kTimestamp; return t;
    };
    static Type Numeric(unsigned length, unsigned precision) {
        Type t;
        t.tclass = kNumeric;
        t.length = length;
        t.precision = precision;
        return t;
    };
    static Type Char(unsigned length) {
        Type t;
        t.tclass = kChar;
        t.length = length;
        return t;
    };
    static Type Varchar(unsigned length) {
        Type t;
        t.tclass = kVarchar;
        t.length = length;
        return t;
    };
    static Type Bool() {
        Type t;
        t.tclass = kBool;
        return t;
    }
};
// ---------------------------------------------------------------------------------------------------
// An index type
enum IndexType: uint8_t {
    kSTLUnorderedMap,
    kSTLMap,
    kSTXMap
};
// ---------------------------------------------------------------------------------------------------
// A single column
// Required by test/schemac/schema_parser_test.cc
struct Column {
    // Name of the column
    std::string id;
    // Type of the column
    Type type;
};
// ---------------------------------------------------------------------------------------------------
// A single table
// Required by test/schemac/schema_parser_test.cc
struct Table {
    // Name of the table
    std::string id;
    // Columns
    std::vector<Column> columns;
    // Primary key column names.
    std::vector<std::string> primary_key;
    // Index type
    IndexType index_type;

    // Get the index of a pk column.
    size_t getPKIndex(std::string_view col_name) const
    {
        for (auto it = columns.cbegin(); it < columns.cend(); ++it) {
            if (it->id == col_name) {
                return std::distance(columns.cbegin(), it);
            }
        }
        throw std::runtime_error("Column " + std::string(col_name) + " does not exist. Type cannot be returned.");
    }

    // Get the type of a relation
    Type getType(std::string_view col_name) const
    {
        for (const auto& col: columns) {
            if (col.id == col_name) {
                return col.type;
            }
        }
        throw std::runtime_error("Column " + std::string(col_name) + " does not exist. Type cannot be returned.");
    }
};
// ---------------------------------------------------------------------------------------------------
// A single index
// Required by test/schemac/schema_parser_test.cc
struct Index {
    // Name of the index
    std::string id;
    // Name of the indexed table
    std::string table_id;
    // Indexes column names.
    std::vector<std::string> columns;
    // Index type
    IndexType index_type;
};
// ---------------------------------------------------------------------------------------------------
// A complete schema
// Required by test/schemac/schema_parser_test.cc
struct Schema {
    // Tables
    std::vector<Table> tables;
    // Indexes
    std::vector<Index> indexes;
};
// ---------------------------------------------------------------------------------------------------
// Schema parse context
class SchemaParseContext {
    friend SchemaParser;

 public:
    // Constructor
    explicit SchemaParseContext(bool trace_scanning = false, bool trace_parsing = false);
    // Destructor
    virtual ~SchemaParseContext();

    // Parse an istream
    Schema Parse(std::istream &in);

    // Throw an error
    void Error(uint32_t line, uint32_t column, const std::string &err);
    // Throw an error
    void Error(const std::string &m);

 private:
    // Begin a scan
    void beginScan(std::istream &in);
    // End a scan
    void endScan();

    // Do a semantic analysis of a given schema.
    void validate(const Schema& schema);

    // Trace the scanning
    bool trace_scanning_;
    // Trace the parsing
    bool trace_parsing_;
};
// ---------------------------------------------------------------------------------------------------
}  // namespace schemac
}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_SCHEMAC_SCHEMA_PARSE_CONTEXT_H_
// ---------------------------------------------------------------------------------------------------

