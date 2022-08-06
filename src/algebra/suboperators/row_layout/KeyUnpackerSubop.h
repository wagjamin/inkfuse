#ifndef INKFUSE_KEYUNPACKERSUBOP_H
#define INKFUSE_KEYUNPACKERSUBOP_H

#include "algebra/suboperators/Suboperator.h"
#include "algebra/suboperators/row_layout/KeyPackerSubop.h"

namespace inkfuse {

/// The key unpacker takes a compound key and serializes data from the compound
/// key at a given offset into an output IU.
struct KeyUnpackerSubop: public TemplatedSuboperator<KeyPackingRuntimeState>, public WithRuntimeParams<KeyPackingRuntimeParams> {
   static SuboperatorArc build(const RelAlgOp* source_, const IU& compound_key_, const IU& target_iu_);

   void setUpStateImpl(const ExecutionContext& context) override;

   void consumeAllChildren(CompilationContext& context) override;

   std::string id() const override;

   private:
   KeyUnpackerSubop(const RelAlgOp* source_, const IU& compound_key_, const IU& target_iu_);
};

}

#endif //INKFUSE_KEYUNPACKERSUBOP_H
