#include "interpreter/FilterFragmentizer.h"
#include "algebra/suboperators/FilterSubop.h"

namespace inkfuse {

/// Filter fragments are special - in a vectorized engine, a filter actually does nothing.
/// The scoping logic which re-installs a new selection vector does all the heavy lifting already.
/// As such, we completely ignore them and generate an empty fragment.
FilterFragmentizer::FilterFragmentizer()
{
   auto& [name, pipe] = pipes.emplace_back();
   name = "FilterSubop";
}

}