#ifndef INKFUSE_KEYPACKERSUBOP_H
#define INKFUSE_KEYPACKERSUBOP_H

#include "algebra/suboperators/Suboperator.h"
#include "algebra/suboperators/properties/WithRuntimeParams.h"
#include "algebra/suboperators/row_layout/KeyPackingRuntimeState.h"

namespace inkfuse {

/// The key packer takes an input IU and writes the data into
/// a compound key at a given offset.
struct KeyPackerSubop : public TemplatedSuboperator<KeyPackingRuntimeState>, public WithRuntimeParams<KeyPackingRuntimeParams> {
   static SuboperatorArc build(const RelAlgOp* source_, const IU& to_pack_, const IU& compound_key_);

   void setUpStateImpl(const ExecutionContext& context) override;

   void consumeAllChildren(CompilationContext& context) override;

   bool isSink() const override { return true; };

   std::string id() const override;

   private:
   KeyPackerSubop(const RelAlgOp* source_, const IU& to_pack_, const IU& compound_key_);
};

}

#endif //INKFUSE_KEYPACKER_H
