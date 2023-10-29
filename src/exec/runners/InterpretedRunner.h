#ifndef INKFUSE_PIPELINEINTERPRETER_H
#define INKFUSE_PIPELINEINTERPRETER_H

#include "PipelineRunner.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace inkfuse {

/// The pipeline intepreter receives a single pipeline
struct InterpretedRunner final : public PipelineRunner {
   /// Create a pipeline interpreter which will interpret the sub-operator at the given index.
   InterpretedRunner(const Pipeline& backing_pipeline, size_t idx, ExecutionContext& original_context);
   ~InterpretedRunner();

   /// Run-morsel override. The InterpretedRunner can interpret some morsels without
   /// actually doing any work.
   Suboperator::PickMorselResult runMorsel(size_t thread_id, bool force_pick) override;

   private:
   /// Get the properly repiped pipeline for the actual execution.
   static PipelinePtr getRepiped(const Pipeline& backing_pipeline, size_t idx);
   /// The fragment id - mainly useful for debugging purposes.
   std::string fragment_id;

   /// How a morsel should be executed.
   enum class ExecutionMode {
      /// Default strategy: simply call the precompiled primitive.
      DefaultRunMorsel,
      /// Optimized zero-copy path for table scans.
      ZeroCopyScan,
   };
   /// Which execution mode `runMorsel` is bound to.
   ExecutionMode mode = ExecutionMode::DefaultRunMorsel;

   /// State required to make the zero copy scan work.
   struct ZeroCopyScanState {
      // IU being provided by the scan.
      const IU* output_iu;
      // Width of the output type.
      size_t type_width;
      // Original pointers to the columns in the output FuseChunk. Might be overwritten
      // and are restored on destruction.
      std::vector<char*> fuse_chunk_ptrs;
   };
   std::optional<ZeroCopyScanState> zero_copy_state;
   /// Custom interpreter for a zero copy scan.
   Suboperator::PickMorselResult runZeroCopyScan(size_t thread_id, bool force_pick);
};
}

#endif //INKFUSE_PIPELINEINTERPRETER_H
