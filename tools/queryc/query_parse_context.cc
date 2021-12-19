//
// Created by benjamin-fire on 20.11.21.
//

#include "query_parse_context.h"
#include "gen/query_parser.h"
#include <stdexcept>
#include <sstream>

imlab::queryc::QueryAST bison_query_parse_result;

namespace imlab {

namespace queryc {

    QueryParseContext::QueryParseContext(bool trace_scanning, bool trace_parsing)
            : trace_scanning_(trace_scanning), trace_parsing_(trace_parsing)
    {
    }

    QueryAST QueryParseContext::Parse(std::istream &in)
    {
        beginScan(in);
        imlab::queryc::QueryParser parser(*this);
        parser.set_debug_level(trace_parsing_);
        parser.parse();
        endScan();

        auto old = std::move(bison_query_parse_result);
        bison_query_parse_result = QueryAST {};

        // Do semantic verification enforcing that the schema definition was valid.
        validate(old);

        return old;
    }

    void QueryParseContext::validate(const QueryAST &query)
    {
    }

    void QueryParseContext::Error(const std::string &m)
    {
        throw std::runtime_error(m);
    }

    void QueryParseContext::Error(uint32_t line, uint32_t column, const std::string &err)
    {
        std::stringstream ss;
        ss << "[ l=" << line << " c=" << column << " ] " << err << std::endl;
        throw std::runtime_error(ss.str());
    }

}

}