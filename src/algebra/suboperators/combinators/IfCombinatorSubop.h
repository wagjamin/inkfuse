#ifndef INKFUSE_IFCOMBINATORSUBOP_H
#define INKFUSE_IFCOMBINATORSUBOP_H

namespace inkfuse {

/// The IfCombinatorSubop provides a suboperator combinator
/// which evaluates the subopator only on rows where a filtered
/// boolean IU is set.
/// This is for example used to only advance hash-table iterators on rows
/// that have not yet found a suitable match.
struct IfCombinatorSubop {

};

}

#endif //INKFUSE_IFCOMBINATORSUBOP_H
