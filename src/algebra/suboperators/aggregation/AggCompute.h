#ifndef INKFUSE_AGGCOMPUTE_H
#define INKFUSE_AGGCOMPUTE_H

namespace inkfuse {

struct CompilationContext;
struct IU;

/// Aggregate function result computation used by an AggReaderSubop.
struct AggCompute {

   /// Virtual base destructor.
   virtual ~AggCompute() = default;

   /// Compute the output IU.
   virtual void compute(CompilationContext& context) = 0;

   private:
   const IU& out_iu;
};

}

#endif //INKFUSE_AGGCOMPUTE_H
