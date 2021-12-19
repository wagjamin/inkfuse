#ifndef IMLAB_QUERY_AST_H
#define IMLAB_QUERY_AST_H

#include <string>
#include <vector>

namespace imlab {
namespace queryc {

    /// Predicate type within the WHERE clause of a query.
    enum class PredType {
        /// Integer Constant.
        IntConstant,
        /// Floating point constant.
        FloatConstant,
        /// String constant.
        StringConstant,
        /// Column reference.
        ColumnRef,
    };

    /// AST being read from a SELECT query
    struct QueryAST {
        /// Columns being read.
        std::vector<std::string> select_list;
        /// Relations being read from.
        std::vector<std::string> from_list;
        /// Equality list containing either constants or attribute names.
        std::vector<std::tuple<std::string, std::string, PredType>> where_list;
    };

}
}

#endif //IMLAB_QUERY_AST_H
