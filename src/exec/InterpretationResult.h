#ifndef INKFUSE_INTERPRETATIONRESULT_H
#define INKFUSE_INTERPRETATIONRESULT_H

#include <cstdint>

namespace inkfuse {

    /// Vectorized database system have a rather unfortunate property. When writing to output chunks in a
    /// vectorized operator, you might simply run out of space.
    /// Don't let the JIT lobbyists fool you - this is a good property. Vectorized chunks should not get too big,
    /// because otherwise you start having a lot of memory consumption in intermediate results. Additionally, there is
    /// a 'sweet spot' when it comes to intermediate result sizes. Check the orginal MonetDB/X100 paper.
    /// Anyways, incremental fusion has the same problems. Because some of our operators are effectively vectorized,
    /// both the vectorized fragments as well as JIT spans over a pipeline can run out of space in the output FuseChunk.
    /// In this case we need to push the output chunk further up the pipeline and can only later return to the remaining
    /// rows on the original operator that ran out of space.
    /// This has to modeled in the generated code. For this, both all code fragments return an InterpretationResult
    /// indicating if there is backpressure on the operator.
    ///
    /// Let's get a bit more philosophical here. Which operators can actually be responsible for this backpressure?
    ///
    enum class InterpretationResult : uint8_t {
        /// There is backpressure on the operator, before we push data into it again, we first have to relieve this
        /// pressure.
        HaveMoreData,
        /// There is no backpressure on the operator, we may feed in a new fuse chunk.
        NeedMoreData,
    };

}

#endif //INKFUSE_INTERPRETATIONRESULT_H
