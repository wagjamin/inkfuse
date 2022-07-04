
#ifndef INKFUSE_WITHLAZYPARAMS_H
#define INKFUSE_WITHLAZYPARAMS_H

#include <optional>

namespace inkfuse {

/// Suboperators that inherit from WithLazyParams behave slightly differently for compiled
/// and vectorized execution. They use one or multiple LazyParams in order to allow for dynamic
/// behaviour that cannot be hard-coded within the primitive. An example is constant addition.
/// We cannot generate code for every constant that might be part of a SQL addition.
template <class Params>
struct WithLazyParams {

   virtual ~WithLazyParams() = default;

   void attachLazyParams(Params lazy_params_) {
      lazy_params = lazy_params_;
   };

   protected:
   std::optional<Params> lazy_params;
};

}

#endif //INKFUSE_WITHLAZYPARAMS_H
