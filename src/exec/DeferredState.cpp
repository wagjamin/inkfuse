#include "exec/DeferredState.h"
#include <cassert>

namespace inkfuse {

TupleMaterializerState::TupleMaterializerState(size_t tuple_size_) : tuple_size(tuple_size_) {
}
void TupleMaterializerState::prepare(size_t num_threads) {
   for (size_t k = 0; k < num_threads; ++k) {
      materializers.push_back(tuple_size);
      handles.push_back(materializers.back().getReadHandle());
   }
}

void* TupleMaterializerState::access(size_t thread_id) {
   assert(thread_id < materializers.size());
   return &materializers[thread_id];
}

} // namespace inkfuse
