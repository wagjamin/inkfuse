#ifndef IMLAB_SEMANA_H
#define IMLAB_SEMANA_H
// ---------------------------------------------------------------------------
#include "query_ast.h"
#include "imlab/algebra/operator.h"
#include "imlab/database.h"
// ---------------------------------------------------------------------------
namespace imlab {
// ---------------------------------------------------------------------------
/// Helpers for semantic analysis.
struct Semana {

    /// Semantic analysis of a parsed query given a schema. Constructs an algebra tree if the result is semantically valid.
    static std::unique_ptr<Operator> analyze(CodegenHelper& codegen, const queryc::QueryAST& ast, const Database& db);
};
// ---------------------------------------------------------------------------
} // namespace imlab
// ---------------------------------------------------------------------------
#endif //IMLAB_SEMANA_H
