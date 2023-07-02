#ifndef INKFUSE_NEWHASHTABLES_H
#define INKFUSE_NEWHASHTABLES_H

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

namespace inkfuse {

/// Comparator for packed keys that can be memcmpared such as (int, float).
struct SimpleKeyComparator {
   SimpleKeyComparator(uint16_t key_size_);

   bool eq(const char* k1, const char* k2) const;
   uint64_t hash(const char* k) const;
   uint16_t keySize() const;

   /// How large is the packed compound key?
   uint16_t key_size;
};

/// Comparator for more complex packed keys such as (string, int, int).
struct ComplexKeyComparator {
   ComplexKeyComparator(uint16_t complex_key_slots_, uint16_t simple_key_size_);

   bool eq(const char* k1, const char* k2) const;
   uint64_t hash(const char* k) const;
   uint16_t keySize() const;

   /// How many 8 byte slots are there at the beginning that need more complex
   /// interpretation logic in the key comparator? (Just string pointers in InkFuse).
   uint16_t complex_key_slots;
   /// How many bytes of simple key that can be memcmpared are following?
   uint16_t simple_key_size;
};

/// A linear probing hash table with an atomic slot tag that can be inserted into in parallel.
/// Used by joins where we need a multi-threaded build phase.
template <class Comparator>
struct AtomicHashTable {
   static const std::string ID;

   AtomicHashTable(Comparator comp_, uint16_t total_slot_size_, size_t num_slots_);

   /// Get the pointer to a given key, or nullptr if the group does not exist.
   char* lookup(const char* key) const;
   /// Get the pointer to a given key, or nullptr if the group does not exist.
   /// If it finds a slot, disables it. Needed for e.g. left semi joins.
   char* lookupDisable(const char* key);
   /// Insert a new key - must only be called when we know the group does not exist yet.
   char* insert(const char* key);

   private:
   /// An iterator within the atomic hash table.
   struct IteratorState {
      /// Index in the linear probing hash table.
      size_t idx;
      /// Key.
      char* data_ptr;
      /// Tag.
      std::atomic<uint8_t>* tag_ptr;
   };
   /// Advances the iterator to the next matching entry.
   inline void lookupInternal(IteratorState& it_state, uint64_t hash, char* key);
   /// Get an iterator to the beginning of the hash table. Points to the first
   /// element in the hash table.
   inline IteratorState itStart() const;
   /// Advance an iterator within the hash table by one slot.
   /// Advance an iterator within the hash table by one slot.
   inline void itAdvance(IteratorState& it) const;
   /// Advance an iterator within the hash table. Sets the pointer to nullptr when the end of the hash table is reached.
   inline void itAdvanceNoWrap(IteratorState& it) const;

   /// The key comparator.
   Comparator comp;
   /// Occupied tags containing parts of the key hash
   /// Similar approach as in folly f14 (just less fast and generic).
   /// This is the only array we need atomic writes to, as threads tag slots
   /// in parallel. Writing to the slots after doesn't have to be atomic.
   std::unique_ptr<std::atomic<uint8_t>[]> tags;
   /// Raw hash table state. Zero key is at the end.
   std::unique_ptr<char[]> data;
   /// How many slots are there?
   uint64_t num_slots;
   /// Index of the last slot, also serves as the modulo mask.
   uint64_t mod_mask;
   /// Total slot size.
   uint16_t total_slot_size;
};

/// A hash table that is owned by just one thread and needs no synchronization.
/// Used by thread-local pre-aggregation.
template <class Comparator>
struct ExclusiveHashTable {
   static const std::string ID;
};

} // namespace inkfuse

#endif //INKFUSE_NEWHASHTABLES_H
