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
   size_t capacity() const { return num_slots; };

   /// Compute the hash for a given key and prefetch the corresponding hash table slot.
   uint64_t compute_hash_and_prefetch(const char* key) const;

   /// Specialization when we use a specific hash width
   /// Specializations exist for 4 and 8 bytes.
   template <uint64_t key_width>
   uint64_t compute_hash_and_prefetch_fixed(const char* key) const;

   /// Prefetch the tag and data slots for a specific hash.
   void slot_prefetch(uint64_t hash) const;
   /// Get the pointer to a given key, or nullptr if the group does not exist.
   /// Already requires the hash was computed.
   char* lookup(const char* key, uint64_t hash) const;
   /// Get the pointer to a given key, or nullptr if the group does not exist.
   /// Already requires the hash was computed.
   /// Variation for outer joins.
   char* lookupOuter(const char* key, uint64_t hash) const;
   /// Get the pointer to a given key, or nullptr if the group does not exist.
   /// If it finds a slot, disables it. Needed for e.g. left semi joins.
   /// Already requires the hash was computed.
   char* lookupDisable(const char* key, uint64_t hash);

   /// Get the pointer to a given key, or nullptr if the group does not exist.
   char* lookup(const char* key) const;
   /// Get the pointer to a given key, or nullptr if the group does not exist.
   /// If it finds a slot, disables it. Needed for e.g. left semi joins.
   char* lookupDisable(const char* key);
   /// Insert a new key - must only be called when we know the group does not exist yet.
   /// @tparam copy_only_key only materialize the key in the hash table. If false copies
   ///                       the payload as well.
   template <bool copy_only_key = true>
   char* insert(const char* key);
   /// Insert variation when we already computed the hash.
   template <bool copy_only_key = true>
   char* insert(const char* key, uint64_t hash);
   /// Insert variation when we already computed the hash.
   template <bool copy_only_key = true>
   char* insertOuter(const char* key, uint64_t hash);

   /// Hash table outer join iterator for init the HashTableSource.
   void iteratorStart(char** it_data, size_t* it_idx);
   /// Hash table outer join iterator advance for the HashTableSource.
   void iteratorAdvance(char** it_data, size_t* it_idx);

   private:
   /// An iterator within the atomic hash table.
   struct IteratorState {
      /// Index in the linear probing hash table.
      uint64_t idx;
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
