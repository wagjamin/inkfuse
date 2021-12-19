//
// Created by benjamin-fire on 20.11.21.
//

#ifndef IMLAB_QUERY_PARSE_CONTEXT_H
#define IMLAB_QUERY_PARSE_CONTEXT_H

#include "query_ast.h"

namespace imlab {
namespace queryc {

    // Query parse context
    class QueryParseContext {
        friend class QueryParser;

    public:
        // Constructor
        explicit QueryParseContext(bool trace_scanning = false, bool trace_parsing = false);
        // Destructor
        virtual ~QueryParseContext() = default;

        // Parse an istream
        QueryAST Parse(std::istream &in);

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
        void validate(const QueryAST& query);

        // Trace the scanning
        bool trace_scanning_;
        // Trace the parsing
        bool trace_parsing_;
    };

}
}

#endif //IMLAB_QUERY_PARSE_CONTEXT_H
