#ifndef INKFUSE_AGGCOMPUTE_H
#define INKFUSE_AGGCOMPUTE_H

#include "codegen/Statement.h"
#include "codegen/Type.h"

namespace inkfuse {

struct CompilationContext;
struct IU;
namespace IR {
struct FunctionBuilder;
}

/// Aggregate function result computation used by an AggReaderSubop.
struct AggCompute {
   /// Virtual base destructor.
   virtual ~AggCompute() = default;

   AggCompute(IR::TypeArc type_);

   /// Compute the aggregate function. Gets a vector of pointers to the underlying aggregate state.
   /// This because the aggregate state may be decomposed into smaller granules.
   virtual IR::StmtPtr compute(IR::FunctionBuilder& builder, const std::vector<IR::Stmt*>& granule_ptrs, const IR::Stmt& out_val) const = 0;

   virtual std::string id() const = 0;

   /// How many ganules does this aggregate consume?
   virtual size_t requiredGranules() const { return 1; };

   protected:
   /// Type leaving the AggCompute.
   IR::TypeArc type;
};

using AggComputePtr = std::unique_ptr<AggCompute>;

}

#endif //INKFUSE_AGGCOMPUTE_H
