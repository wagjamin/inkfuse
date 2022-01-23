#include "FuseChunkSink.h"
#include "exec/FuseChunk.h"
#include "runtime/Runtime.h"

namespace inkfuse {

FuseChunkSink::FuseChunkSink(FuseChunkSinkParameters params_, IURef read_iu_, FuseChunk& chunk_)
   : params(std::move(params_)), read_iu(read_iu_), chunk(chunk_) {
}

void FuseChunkSink::compile(CompilationContext& context, const std::set<IURef>& required) const {
}

void FuseChunkSink::setUpState() {
   auto& col = chunk.getColumn(read_iu);
   // Empty out column, we don't want a new query to write into pre-filled buffers.
   col.size = 0;
   state = std::make_unique<FuseChunkSinkState>(FuseChunkSinkState{
      .data= &col,
   });
}

void FuseChunkSink::tearDownState() {
   state.reset();
}

void *FuseChunkSink::accessState() const
{
    return state.get();
}

std::string FuseChunkSink::id() const
{
    /// Only have to parametrize over the type.
    return "column_scan_" + params.type->id();
}

void FuseChunkSink::registerRuntime()
{
    /// FuseChunkSinkState needs to be accessible from within the generated code.
    RuntimeStructBuilder("FuseChunkSinkState")
        .addMember("data", IR::Pointer::build(IR::Void::build()));
}

}
