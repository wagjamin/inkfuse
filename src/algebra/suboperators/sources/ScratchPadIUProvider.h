#include "algebra/suboperators/IndexedIUProvider.h"

namespace inkfuse {

struct RelAlgOp;

/// The ScratchPadIUProvider is a special IU provider which creates space for consuming suboperators
/// in the global scope of the generated function.
/// This is useful in cases where the IU is not read from storage, but rather transient and used for intermediate state.
/// This is most commonly used for key packing/unpacking, where we don't need to allocate a complete column
/// in the context of a compiled pipeline.
struct ScratchPadIUProvider : public TemplatedSuboperator<EmptyState>, public WithRuntimeParams<IndexedIUProviderRuntimeParam> {

   ScratchPadIUProvider(RelAlgOp* source_, const IU& provided_iu);

   /// The ScratchPadIUProvider does not have to be interpreted.
   /// For operators which consume this in an interpreted context,
   /// it gets turned into a simple regular IU on a FuseChunk.
   bool outgoingStrongLinks() const override {
      return true;
   }

   /// The ScratchPadIUProvider is _not_ a source. This is because it should not
   /// be retained during a repipe operation.
   /// While we e.g. need to retain the loop driver of a table scan, the scratch pad scan
   /// should be turned into a regular fuse chunk operation.
   bool isSource() const override {
      return false;
   }

   std::string id() const override {
      return "ScratchPadIUProvider";
   };

   void consume(const IU& iu, CompilationContext& context) override;

};

} // namespace inkfuse
