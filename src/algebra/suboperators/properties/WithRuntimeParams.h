
#ifndef INKFUSE_WITHRUNTIMEPARAMS_H
#define INKFUSE_WITHRUNTIMEPARAMS_H

#include <optional>

namespace inkfuse {

/// Suboperators that inherit from WithRuntimeParams behave slightly differently for compiled
/// and vectorized execution. They use one or multiple RUNTIME_PARAMs in order to allow for dynamic
/// behaviour that cannot be hard-coded within the primitive. An example is constant addition.
/// We cannot generate code for every constant that might be part of a SQL addition.
template <class Params>
struct WithRuntimeParams {

   virtual ~WithRuntimeParams() = default;

   void attachRuntimeParams(Params runtime_params_) {
      runtime_params = std::move(runtime_params_);
   };

   protected:
   Params runtime_params;
};

}

#endif //INKFUSE_WITHRUNTIMEPARAMS_H
