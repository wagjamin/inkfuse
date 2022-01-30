#ifndef INKFUSE_RELALGOP_H
#define INKFUSE_RELALGOP_H

#include "algebra/IU.h"
#include <set>

namespace inkfuse {

    struct PipelineDAG;

    /// Relational algebra operator producing a set of IUs. Presents a single SQL query at the
    /// relational operator.
    /// As we prepare a relational algebra query for execution, it "decays" into a DAG suitable of suboperators.
    ///
    /// Let's look at this from a compiler perspective - inkfuse is effectively a compiler lowering to progressively
    /// lower level IRs. relational algebra level -> suboperators -> inkfuse IR -> C.
    /// This would be pretty sweet to build into MLIR - but we decided against it because we thought the
    /// initial overhead might be overkill.
    struct RelalgOp {

        void decay(std::set<IURef> required, PipelineDAG& dag);

    };

}


#endif //INKFUSE_RELALGOP_H
