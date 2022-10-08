#include "algebra/suboperators/IndexedIUProvider.h"

namespace inkfuse {

struct RelAlgOp;

/// The ScratchPadIUProvider is a special IU provider which creates space for consuming suboperators
/// in the global scope of the generated function.
/// This is useful in cases where the IU is not read from storage, but rather transient and used for intermediate state.
/// This is most commonly used for key packing/unpacking, where we don't need to allocate a complete column
/// in the context of a compiled pipeline.
struct ScratchPadIUProvider : public TemplatedSuboperator<EmptyState> {

   static SuboperatorArc build(RelAlgOp* source_, const IU& provided_iu);

   /// The ScratchPadIUProvider does not have to be interpreted.
   /// For operators which consume this in an interpreted context,
   /// it gets turned into a simple regular IU on a FuseChunk.
   bool outgoingStrongLinks() const override {
      return true;
   }

   std::string id() const override {
      return "ScratchPadIUProvider";
   };

   /// As the ScratchPadIUProvider has no source, the code generation has to happen in 'open'.
   void open(CompilationContext& context) override;

   /// Custom 'close' as we don't need to look for IU producers to also close.
   void close(CompilationContext& context) override {};

   private:
   ScratchPadIUProvider(RelAlgOp* source_, const IU& provided_iu);

};

} // namespace inkfuse
