#ifndef INKFUSE_TUPLEMATERIALIZER_H
#define INKFUSE_TUPLEMATERIALIZER_H

#include <atomic>
#include <cstdint>
#include <deque>
#include <memory>

namespace inkfuse {

/// The TupleMaterializer is a thread-local structure that allows materializing tuples in
/// a row-layout. It is the sink for join-build pipelines. The materialization buffers
/// are later used in a multi-threaded build phase of the actual hash table.
struct TupleMaterializer {
   public:
   TupleMaterializer(size_t tuple_size_);

   /// Get a slot for materializing a fully packed tuple into the buffers of the TupleMaterializer.
   char* materialize();

   /// A chunk being materialized.
   struct MatChunk {
      char* end_ptr = nullptr;
      /// 8 byte aligned data.
      std::unique_ptr<uint64_t[]> data;
   };

   /// A thread-safe read handle once we start reading again.
   /// Native support for morsel-picking.
   struct ReadHandle {
      /// Get a new chunk to read, or nullptr if all chunks were read already.
      const MatChunk* pullChunk();

      private:
      ReadHandle(const TupleMaterializer& mat_) : mat(mat_){};

      /// Offset in the chunk deque.
      std::atomic<size_t> offset = 0;
      const TupleMaterializer& mat;

      friend class TupleMaterializer;
   };

   /// Extract a ReadHandle to read the materialized data again.
   std::unique_ptr<ReadHandle> getReadHandle() const;

   /// How many tuples were materialized into the buffers?
   size_t getNumTuples() const;

   private:
   /// Create a new chunk and set it as the current one.
   void allocNewChunk();

   /// Size of the tuples being materialized.
   size_t tuple_size;
   /// Efficient access into the current chunk being populated.
   char** curr_chunk_end_ptr_ptr = nullptr;
   char* curr_chunk_end = nullptr;
   /// Size of the tuples being materialized.
   std::deque<MatChunk> chunks;
   /// The offset in the current chunk we are writing to.
   size_t curr_chunk_offset = 0;

   friend class ReadHandle;
};

namespace TupleMaterializerRuntime {
extern "C" char* materialize_tuple(void* materializer);

void registerRuntime();
}

} // namespace inkfuse

#endif // INKFUSE_TUPLEMATERIALIZER_H
