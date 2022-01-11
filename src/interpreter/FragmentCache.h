#ifndef INKFUSE_FRAGMENTCACHE_H
#define INKFUSE_FRAGMENTCACHE_H

namespace inkfuse {

/// The FragmentCache loads the shared object created by the FragmentGenerator during the initial compilation of
/// the inkfuse binary. The FragmentCache then provides fast access to these pre-compiled snippets for use within
/// the vectorized interpreter of the QueryExecutor.
struct FragmentCache {
};

} // namespace inkfuse

#endif //INKFUSE_FRAGMENTCACHE_H
