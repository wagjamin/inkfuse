#include "runtime/TupleMaterializer.h"
#include "runtime/Runtime.h"
#include <cstring>

namespace inkfuse {

namespace {
/// We currently allocate 16kb chunks - i.e. one page.
const uint64_t REGION_SIZE = 32 * 512;
} // namespace

TupleMaterializer::TupleMaterializer(size_t tuple_size_) : tuple_size(tuple_size_) {
   if (tuple_size_ > REGION_SIZE) {
      throw std::runtime_error("Tuple size in TupleMaterializer must be less than 4kb");
   }
   allocNewChunk();
}

void TupleMaterializer::materialize(const char* tuple_) {
   if (*curr_chunk_end_ptr_ptr + tuple_size >= curr_chunk_end) {
      // Tuple doesn't fit anymore - make space for a new one.
      allocNewChunk();
   }
   std::memcpy(*curr_chunk_end_ptr_ptr, tuple_, tuple_size);
   *curr_chunk_end_ptr_ptr += tuple_size;
}

std::unique_ptr<TupleMaterializer::ReadHandle> TupleMaterializer::getReadHandle() const {
   return std::unique_ptr<TupleMaterializer::ReadHandle>(new ReadHandle(*this));
}

const TupleMaterializer::MatChunk* TupleMaterializer::ReadHandle::pullChunk() {
   const size_t read_offset = offset.fetch_add(1);
   if (read_offset >= mat.chunks.size()) {
      return nullptr;
   }
   return &mat.chunks[read_offset];
}

void TupleMaterializer::allocNewChunk() {
   // Set up the new chunk.
   auto& chunk = chunks.emplace_back();
   chunk.data = std::make_unique<uint64_t[]>(REGION_SIZE / 8);

   // Update local state to materialize into that chunk now.
   char* as_char = reinterpret_cast<char*>(chunk.data.get());
   chunk.end_ptr = as_char;
   curr_chunk_end_ptr_ptr = &chunk.end_ptr;
   curr_chunk_end = as_char + REGION_SIZE;
}

namespace TupleMaterializerRuntime {

extern "C" void materialize_tuple(void* materializer, char* key) {
   reinterpret_cast<TupleMaterializer*>(materializer)->materialize(key);
}

void registerRuntime() {
   RuntimeFunctionBuilder("materialize_tuple", IR::Void::build())
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);
}

} // namespace TupleMaterializerRuntime

} // namespace inkfuse
