#ifndef INKFUSE_FRAGMENTCACHE_H
#define INKFUSE_FRAGMENTCACHE_H

#include "codegen/IR.h"
#include "codegen/backend_c/BackendC.h"

#include <string>

namespace inkfuse {

/// Singleton helper.
template<class T>
struct Singleton {

   static T& instance() {
      static T obj;
      return obj;
   };
};

/// The FragmentCache loads the shared object created by the FragmentGenerator during the initial compilation of
/// the inkfuse binary. The FragmentCache then provides fast access to these pre-compiled snippets for use within
/// the vectorized interpreter of the QueryExecutor.
struct FragmentCache : public Singleton<FragmentCache> {

   /// Get the backing function of a given fragment name.
   void* getFragment(std::string_view name);

   private:
   /// Fragment implementation which gets linked dynamically.
   std::unique_ptr<IR::BackendProgram> program;

   protected:
   friend class Singleton<FragmentCache>;

   FragmentCache();
};

} // namespace inkfuse

#endif //INKFUSE_FRAGMENTCACHE_H
