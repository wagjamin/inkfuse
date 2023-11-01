#include "InterpretedRunner.h"
#include "algebra/suboperators/row_layout/KeyPackerSubop.h"
#include "algebra/suboperators/sources/TableScanSource.h"
#include "interpreter/FragmentCache.h"

namespace inkfuse {

InterpretedRunner::InterpretedRunner(const Pipeline& backing_pipeline, size_t idx, ExecutionContext& original_context, bool pick_from_source_table_)
   : PipelineRunner(getRepiped(backing_pipeline, idx), original_context), pick_from_source_table(pick_from_source_table_) {
   // Get the unique identifier of the operation which has to be interpreted.
   auto& op = backing_pipeline.getSubops()[idx];
   fragment_id = op->id();
   // Get the function we have to interpret.
   auto& cache = FragmentCache::instance();
   fct = reinterpret_cast<uint8_t (*)(void**)>(cache.getFragment(fragment_id));
   if (!fct) {
      throw std::runtime_error("No fragment " + fragment_id + " found for interpreted runner.");
   }
   prepared = true;

   if (dynamic_cast<TScanIUProvider*>(op.get())) {
      // This is actually an IU provider.
      mode = ExecutionMode::ZeroCopyScan;
      zero_copy_state = ZeroCopyScanState{
         .output_iu = op.get()->getIUs().at(0),
         .type_width = op.get()->getIUs().at(0)->type->numBytes(),
         .fuse_chunk_ptrs{original_context.getNumThreads()},
      };
   }
}

InterpretedRunner::~InterpretedRunner() {
   // If we overwrote FuseChunk column pointers restore them now.
   if (zero_copy_state) {
      for (size_t thread_id = 0; thread_id < context.getNumThreads(); ++thread_id) {
         if (zero_copy_state->fuse_chunk_ptrs[thread_id] != nullptr) {
            context.getColumn(*zero_copy_state->output_iu, thread_id).raw_data = zero_copy_state->fuse_chunk_ptrs[thread_id];
         }
      }
   }
}

Suboperator::PickMorselResult InterpretedRunner::pickMorsel(size_t thread_id) {
   if (mode == ExecutionMode::ZeroCopyScan && !pick_from_source_table) {
       // If this is a suboperator interpreting a table scan, but not the first one,
       // then we should not pick from the table scan. We need to bind to the original
       // morsel of the first interpreter instead.
      return Suboperator::PickedMorsel{
         .morsel_size = 1,
      };
   }

   // Pick a morsel from the source suboperator.
   auto morsel = pipe->suboperators[0]->pickMorsel(thread_id);

   if (auto picked = std::get_if<Suboperator::PickedMorsel>(&morsel); picked) {
      // FIXME - HACKFIX - Tread With Caution
      // It could be that we require scratch pad IUs within this pipeline.
      // As these are never officially produced by an expression, we need
      // to make sure their sizes are set up properly.
      for (const auto& subop : pipe->getSubops()) {
         if (dynamic_cast<KeyPackerSubop*>(subop.get())) {
            assert(subop->getSourceIUs().size() == 2);
            const IU* scratch_pad_iu = subop->getSourceIUs()[1];
            context.getColumn(*scratch_pad_iu, thread_id).size = picked->morsel_size;
         }
      }
   }

   return morsel;
}

void InterpretedRunner::runMorsel(size_t thread_id) {
   // Dispatch to the right interpretation strategy.
   switch (mode) {
      case ExecutionMode::DefaultRunMorsel:
         // Default strategy.
         PipelineRunner::runMorsel(thread_id);
         break;
      case ExecutionMode::ZeroCopyScan:
         // Custom zero-copy scan interpreter.
         runZeroCopyScan(thread_id);
         break;
   }
}

void InterpretedRunner::runZeroCopyScan(size_t thread_id) {
   // The TScanIUProvider used to make a superfluous copy of the table scan source.
   // We would take the table scan data, and do a contiguous copy of the data into a
   // FuseChunk.
   // An alternative is to just take the pointer we get from the table scan morsel
   // and hook that up as the output of the operation.

   // Only the first TScanIUProvider needs to actually pick the new morsel.
   // That is marked with `force_pick`. In all other cases assume we already
   // have a morsel picked.
   if (zero_copy_state->fuse_chunk_ptrs[thread_id] == nullptr) {
      // Save the raw state of the fuse chunk column. Will be reinstalled on Runner destruction.
      zero_copy_state->fuse_chunk_ptrs[thread_id] = context.getColumn(*zero_copy_state->output_iu, thread_id).raw_data;
   }
   // Get the state of the picked morsel.
   TScanDriver* driver = dynamic_cast<TScanDriver*>(pipe->suboperators[0].get());
   TScanIUProvider* provider = dynamic_cast<TScanIUProvider*>(pipe->suboperators[1].get());
   assert(driver);
   assert(provider);
   const auto driver_state = reinterpret_cast<LoopDriverState*>(driver->accessState(thread_id));
   const auto provider_state = reinterpret_cast<IndexedIUProviderState*>(provider->accessState(thread_id));
   // And directly update the target fuse chunk column.
   Column& out_col = context.getColumn(*zero_copy_state->output_iu, thread_id);
   out_col.size = (driver_state->end - driver_state->start);
   out_col.raw_data = (*provider_state->start) + (driver_state->start * zero_copy_state->type_width);
}

// static
PipelinePtr InterpretedRunner::getRepiped(const Pipeline& backing_pipeline, size_t idx) {
   auto res = backing_pipeline.repipeAll(idx, idx + 1);
   return res;
}
}
