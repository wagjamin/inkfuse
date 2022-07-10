#ifndef INKFUSE_COPY_H
#define INKFUSE_COPY_H

#include "algebra/suboperators/Suboperator.h"

namespace inkfuse {

/// Copies one IU into another. A very boring sub-operator, but a nice
/// demo and also valuable for testing.
struct Copy final : public TemplatedSuboperator<EmptyState> {
   static std::unique_ptr<Copy> build(const IU& copy_iu);

   void consume(const IU& iu, CompilationContext& context) override;

   std::string id() const override;

   private:
   /// Constructor taking the IU to be copied.
   Copy(const IU& copy_iu);

   /// The produced copy iu.
   const IU produced;
};

}

#endif //INKFUSE_COPY_H
