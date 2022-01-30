#ifndef INKFUSE_EXECUTIONCONTEXT_H
#define INKFUSE_EXECUTIONCONTEXT_H

#include <cstdint>
#include <memory>
#include <unordered_map>
#include "exec/FuseChunk.h"

namespace inkfuse {

struct Suboperator;
struct IU;

/// In inkfuse, a scope represents the maximum number of suboperators during whose execution
/// a selection vector remains valid. When leaving a scope, a new FuseChunk and a new selection vector
/// are introduced. As such, all suboperators which do not necessarily represent a 1:1 mapping between
/// an input and and output tuple generate a new scope.
/// Note that this corresponds very closely to actual scopes in the generated code.
struct Scope {
    /// Create a new scope which re-uses an old chunk.
    Scope(size_t id_, std::shared_ptr<FuseChunk> chunk_):
            selection(std::make_unique<IU>(IR::Bool::build(), "sel_scope" + std::to_string(id))),
            id(id_),
            chunk(std::move(chunk_))
    {
        // We need a column containing the current selection vector.
        chunk->attachColumn(*selection);
    }

    /// Create a completely new scope with a clean chunk.
    Scope(size_t id_): Scope(id_, std::make_shared<FuseChunk>()) {};

    /// Register an IU producer within the given scope.
    void registerProducer(IURef iu, Suboperator& op);

    /// Get the producing sub-operator of an IU within the given scope.
    Suboperator* getProducer(IURef iu);

    /// Scope id.
    size_t id;
    /// Boolean IU for the selection vector. We need a pointer to retain IU stability.
    std::unique_ptr<IU> selection;
    /// Backing chunk of data within this scope. For efficiency reasons, a new scope somtimes can refer
    /// to the same backing FuseChunk as the previous ones. This is done to ensure that a filter sub-operator
    /// does not have to copy all data.
    std::shared_ptr<FuseChunk> chunk;
    /// A map from IUs to the producing operators.
    std::unordered_map<IU*, Suboperator*> iu_producers;
};

using ScopePtr = std::unique_ptr<Scope>;

/// The execution context for a single pipeline.
struct ExecutionContext {

    /// The scopes of the backing DAG within a single pipeline.
    std::unordered_map<size_t, ScopePtr> scopes;

};

}

#endif //INKFUSE_EXECUTIONCONTEXT_H
