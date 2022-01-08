#ifndef INKFUSE_FUSECHUNKSINK_H
#define INKFUSE_FUSECHUNKSINK_H

#include "algebra/Suboperator.h"

namespace inkfuse {

    struct FuseChunk;
    struct Column;

    /// State needed to get a fuse chunk sink to operate on underlying data. Provided at runtime.
    struct FuseChunkSinkState {
        /// Pointer to the column into which we write. This is a FuseChunk.
        /// But retaining it as a void* within the generated code is easier because
        /// otherwise we have to worry about declaration order in our global preamble.
        void* column;
    };

    struct FuseChunkSinkParameters {
        /// Type being written by this FuseChunkSink.
        IR::TypeArc type;
    };

    /// Writes a single IU into a fuse chunk.
    struct FuseChunkSink : public Suboperator {

        /// Set up a new fuse chunk sink.
        FuseChunkSink(FuseChunkSinkParameters params_, IURef read_iu_, FuseChunk& chunk_);

        void compile(CompilationContext& context, const std::set<IURef>& required) override;

        void setUpState() override;

        void tearDownState() override;

    private:
        /// Backing parameters.
        FuseChunkSinkParameters params;
        /// IU being read by this sink and written into the chunk.
        IURef read_iu;
        /// Fuse chunk into which we write.
        const FuseChunk& chunk;
        /// Backing runtime state for the compiled code.
        std::unique_ptr<FuseChunkSinkState> state;
    };

}

#endif //INKFUSE_FUSECHUNKSINK_H
