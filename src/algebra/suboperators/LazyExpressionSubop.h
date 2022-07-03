#ifndef INKFUSE_LAZYEXPRESSIONSUBOP_H
#define INKFUSE_LAZYEXPRESSIONSUBOP_H

#include "algebra/suboperators/Suboperator.h"
#include "algebra/suboperators/LazyParam.h"
#include "algebra/suboperators/properties/WithLazyParams.h"

namespace inkfuse {

/// Runtime state of a given lazy expression suboperator.
struct LazyExpressionSubopState {
   static const char* name;

   /// Type-erased pointer to a backing value. This way we don't have to define LazyExpressionSubops
   /// for every value.
   void* data_erased;
};

struct LazyExpressionParams {
   LAZY_PARAM(data, LazyExpressionSubopState);
};

struct LazyExpressionSubop : public TemplatedSuboperator<LazyExpressionSubop, EmptyState>, WithLazyParams<LazyExpressionParams> {

};

} // namespace inkfuse

#endif //INKFUSE_LAZYEXPRESSIONSUBOP_H
