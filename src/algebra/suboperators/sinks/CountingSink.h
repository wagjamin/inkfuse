#ifndef INKFUSE_COUNTINGSINK_H
#define INKFUSE_COUNTINGSINK_H

#include "algebra/suboperators/Suboperator.h"

namespace inkfuse {

struct CountingState {
   static const char* name;

   uint64_t count = 0;
};

struct CountingSink : public TemplatedSuboperator<CountingState> {

   static SuboperatorArc build(const IU& input_iu);

   CountingSink(const IU& input_iu);

   bool isSink() const override { return true; }

   void consume(const IU& iu, CompilationContext& context) override;

   void setUpStateImpl(const ExecutionContext& context) override;

   std::string id() const override;

   /// Register runtime structs and functions.
   static void registerRuntime();
};

}

#endif //INKFUSE_COUNTINGSINK_H
