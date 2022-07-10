#ifndef INKFUSE_HASHTABLERUNTIME_H
#define INKFUSE_HASHTABLERUNTIME_H

#include "codegen/Type.h"

namespace inkfuse {

/// Both aggregations, as well as joins need to use hash tables in order to compute their result sets.
/// Inkfuse makes this possible by exposing a HashTableRuntime to the generated code.
///
/// The has table provides the following interface:
/// lookup(void* table, uint64_t hash) -> char* data
/// lookupNext(void* table, char* data) -> char* data
/// insert(void* table, uint64_t hash) -> char* data
///
/// As such, the hash table itself has no concept of the underlying key types.
/// Rather, the suboperator primitives have to do key equality checking.
namespace HashTableRuntime {

extern "C" char* ht_lookup(void* table, uint64_t hash);
extern "C" char* ht_lookupNext(void* table, char* data);
extern "C" char* ht_insert(void* table, uint64_t hash);

void registerRuntime();
};

/// A hash table that that is used by the runtime.
/// We cannot make this one generic as key/value combinations can be arbitrary.
///
/// This is a simple linear-probing hashtable, every slot in the table has the
/// following layout:
/// [set (1 bit) | hash (63 bits) | key (key_size bytes) | value (val_size bytes) ]
///
/// TODO this can be made more fancy, but is good enough for v0.
struct HashTable {
   public:
   HashTable(uint16_t payload_size_, size_t start_size_ = 2048);

   /// Get iterator to a given hash.
   char* lookup(uint64_t hash);
   /// Move iterator to the next value or return null if no more matching hashes.
   char* lookupNext(char* data);
   /// Insert a new element and return .
   char* insert(uint64_t hash);

   private:
   /// Get the next index.
   inline char* advanceIndex(char* crr);
   /// Resolve the collision chain at the current slot by moving to the next element with the same hash.
   inline char* resolveChain(uint64_t hash, char* curr);
   /// Find the next free slot in the table.
   inline char* nextFreeSlot(char* curr);
   /// Double the table size and reinsert elements.
   void doubleSize();

   /// Raw hash table state.
   std::unique_ptr<char[]> data;
   /// Pointer to the last slot.
   char* last_index;
   /// Modulo mask for this table.
   uint64_t mod_mask;
   /// Current number of inserted elements.
   size_t inserted = 0;
   /// Allowed maximum number of elements before resize.
   size_t max_fill;
   /// Current number of slots.
   size_t slots = 0;
   /// Size of every slot (is 8 + payload_size_).
   uint16_t slot_size;
};

}

#endif //INKFUSE_HASHTABLERUNTIME_H
