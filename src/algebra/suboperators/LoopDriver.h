#ifndef INKFUSE_LOOPDRIVER_H
#define INKFUSE_LOOPDRIVER_H

#include "algebra/suboperators/Suboperator.h"

namespace inkfuse {

struct LoopDriverState {
   /// Start of the loop variable.
   uint64_t start;
   /// End of the loop variable.
   uint64_t end;
};

struct LoopDriver : public Suboperator
{
   /// Constructor;
   LoopDriver(const Operator& source_, Pipeline& pipe_, JobPtr job_);

   void produce(CompilationContext& context) const override;
   void consume(Suboperator& child) const override;
   void interpret() const override;

   /// The LoopDriver is a source operator which picks morsel ranges from the backing storage.
   bool isSource() override { return true; }

   ///
   bool pickMorsel() override { }

   /// Register the runtime needed for the loop driver.
   static void registerRuntime();

   private:
   /// The state of this sub-operator.
   std::unique_ptr<LoopDriverState> state;
   /// The job which initialized the morsel (LoopDriverState).
   JobPtr job;
};

}

#endif //INKFUSE_LOOPDRIVER_H
