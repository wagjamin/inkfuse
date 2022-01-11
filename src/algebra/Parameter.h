#ifndef INKFUSE_PARAMETER_H
#define INKFUSE_PARAMETER_H

#include "codegen/Type.h"
#include <hash_set>

namespace inkfuse {

/// A discrete parameter which can only take a specific set of values.
/// Typical example within SQL are a subset of types.
struct DiscreteParameter {
};

struct DiscreteSetParameter : public DiscreteParameter {
};

struct InfiniteParameter {
};

}

#endif //INKFUSE_PARAMETER_H
