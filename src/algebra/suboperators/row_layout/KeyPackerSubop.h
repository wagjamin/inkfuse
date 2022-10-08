#ifndef INKFUSE_KEYPACKERSUBOP_H
#define INKFUSE_KEYPACKERSUBOP_H

#include "algebra/suboperators/Suboperator.h"
#include "algebra/suboperators/properties/WithRuntimeParams.h"
#include "algebra/suboperators/row_layout/KeyPackingRuntimeState.h"

namespace inkfuse {

namespace KeyPacking {
/// Key packing helper function.
IR::StmtPtr packInto(IR::ExprPtr ptr, IR::ExprPtr to_pack, IR::ExprPtr offset);
}

/// The key packer takes an input IU and writes the data into
/// a compound key at a given offset.
/// The KeyPackerSubop can accept a set of pseudo-IUs if you want to force that other suboperators come after.
/// A typical example for this is when you do key-packing into a scratchpad IU which you later want to 
/// insert into a hash table.
struct KeyPackerSubop : public TemplatedSuboperator<KeyPackingRuntimeState>, public WithRuntimeParams<KeyPackingRuntimeParams> {
   static SuboperatorArc build(const RelAlgOp* source_, const IU& to_pack_, const IU& compound_key_, std::vector<const IU*> pseudo_ius = {});

   void setUpStateImpl(const ExecutionContext& context) override;

   void consumeAllChildren(CompilationContext& context) override;

   std::string id() const override;

   private:
   KeyPackerSubop(const RelAlgOp* source_, const IU& to_pack_, const IU& compound_key_, std::vector<const IU*> pseudo_ius = {});
};

}

#endif //INKFUSE_KEYPACKER_H
