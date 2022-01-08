#include "algebra/FuseChunkSink.h"
#include "exec/FuseChunk.h"

namespace inkfuse {

    FuseChunkSink::FuseChunkSink(FuseChunkSinkParameters params_, IURef read_iu_, FuseChunk &chunk_)
    : params(std::move(params_)), read_iu(read_iu_), chunk(chunk_)
    {

    }

    void FuseChunkSink::compile(CompilationContext &context, const std::set<IURef> &required)
    {
    }

    void FuseChunkSink::setUpState()
    {
        auto& col = chunk.getColumn(read_iu);
        // Empty out column, we don't want a new query to write into pre-filled buffers.
        col.size = 0;
        state = std::make_unique<FuseChunkSinkState>(FuseChunkSinkState{
            .column = &col,
                });
    }

    void FuseChunkSink::tearDownState()
    {
        state.reset();
    }

}
