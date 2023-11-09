#ifndef INKFUSE_HASHTABLES_H
#define INKFUSE_HASHTABLES_H

#include <cstdint>
#include <deque>
#include <memory>

/// This file contains the single-threaded hash tables used for aggregating data in InkFuse.
namespace inkfuse {

// TODO(benjamin) - lots of code duplication here for our single-threaded hash tables/
//                  Should be cleaned up in a similar way to the AtomicHashTables.

/// Shared state between the different hash table implementations.
struct SharedHashTableState {
   SharedHashTableState(uint16_t total_slot_size_, size_t start_slots_);

   /// Advance an iterator within the hash table.
   inline void advance(size_t& idx, char*& curr, uint8_t*& tag) const;
   /// Advance an iterator within the hash table. Sets the pointer to nullptr when the end of the hash table is reached.
   inline void advanceNoWrap(size_t& idx, char*& curr, uint8_t*& tag) const;
   /// Advance an iterator within the hash table. Sets the pointer to nullptr when the end of the hash table is reached.
   inline void advanceWithJump(size_t& idx, char*& curr, uint8_t*& tag, uint64_t& jump_point, size_t num_threads) const;

   /// Occupied tags containing parts of the key hash.
   /// Similar approach as in folly f14 (just less fast and generic).
   std::unique_ptr<uint8_t[]> tags;
   /// Raw hash table state. Zero key is at the end.
   std::unique_ptr<char[]> data;
   /// Index of the last slot, also serves as the modulo mask.
   uint64_t mod_mask;
   /// Current number of inserted elements.
   size_t inserted = 0;
   /// Allowed maximum number of elements before resize.
   size_t max_fill;
   /// Total slot size.
   uint16_t total_slot_size;
};

/// A hash table where key equality checks can be performed as a simple memcmp.
struct alignas(64) HashTableSimpleKey {
   /// Unique Hash Table ID.
   static const std::string ID;

   HashTableSimpleKey(uint16_t key_size_, uint16_t payload_size_, size_t start_slots_ = 2048);
   static size_t getMergeTableSize(
      std::deque<std::unique_ptr<HashTableSimpleKey>>& preagg, size_t thread_count);

   /// Compute the hash of some serialized key.
   uint64_t computeHash(const char* key) const;
   /// Get the pointer to a given key, or nullptr if the group does not exist.
   char* lookup(const char* key);
   /// Get the pointer to a given key, or nullptr if the group does not exist.
   /// If it finds a slot, disables it. Needed for e.g. left semi joins.
   char* lookupDisable(const char* key);
   /// Get the pointer to a given key, creating a new group if it does not exist yet.
   char* lookupOrInsert(const char* key);
   /// Insert a new key - must only be called when we know the group does not exist yet.
   char* insert(const char* key);
   /// Get the pointer to a given key, creating a new group if it does not exist yet.
   /// Updates the 'result' and 'is_new_key' arguments to give the caller
   /// insight into whether this key was new.
   void lookupOrInsert(char** result, bool* is_new_key, const char* key);
   /// Get an iterator to the first non-empty element of the hash table.
   /// Sets it_data to nullptr if the iterator is exhausted.
   void iteratorStart(char** it_data, uint64_t* it_idx);
   /// Advance an iterator to the next non-empty element in the hash table.
   /// Sets it_data to nullptr if the iterator is exhausted.
   void iteratorAdvance(char** it_data, uint64_t* it_idx);
   /// Get the current size. Mainly used for testing.
   size_t size() const;
   /// Get the current capacity. Mainly used for testing.
   size_t capacity() const;

   /// Efficient iterator that allows iterating over only some hash bit
   /// partitions.
   void partitionedIteratorStart(
           char** it_data, 
           uint64_t* it_idx,
           uint64_t* next_jump_point,
           size_t num_threads, 
           size_t thread_id);
   void partitionedIteratorAdvance(
           char** it_data, 
           uint64_t* it_idx,
           uint64_t* next_jump_point,
           size_t num_threads, 
           size_t thread_id);


   size_t keySize() const { return simple_key_size; }
   size_t payloadSize() const { return state.total_slot_size - simple_key_size; }

   /// Special function if we know this hash table is only ever called with a single key.
   char* lookupOrInsertSingleKey();

   private:
   struct LookupResult {
      char* elem;
      uint8_t* tag;
   };

   /// Find the correct slot for the key, or the first one which is empty.
   inline LookupResult findSlotOrEmpty(uint64_t hash, const char* string);
   /// Find the first empty slot for the given hash.
   inline LookupResult findFirstEmptySlot(uint64_t hash);
   /// Make sure one more slot can be added to the hash table.
   /// If not, doubles size.
   void reserveSlot();

   SharedHashTableState state;
   /// Size of the materialized simple key.
   uint16_t simple_key_size;
};

/// A hash table with a more complex key. In principle, the key can contain both
/// a simple part which can just be memcmpared, as well as a set of
/// successive key slots for more complex data types (such as strings).
/// Currently we only support using a single string as key.
struct alignas(64) HashTableComplexKey {
   /// Unique Hash Table ID.
   static const std::string ID;

   HashTableComplexKey(uint16_t simple_key_size, uint16_t complex_key_slots, uint16_t payload_size, size_t start_slots = 64);
   static size_t getMergeTableSize(
      std::deque<std::unique_ptr<HashTableComplexKey>>& preagg, size_t thread_count);

   /// Compute the hash of some serialized key.
   uint64_t computeHash(const char* key) const;
   /// Get the pointer to a given key, or nullptr if the group does not exist.
   char* lookup(const char* key);
   /// Get the pointer to a given key, creating a new group if it does not exist yet.
   char* lookupOrInsert(const char* key);
   /// Get the pointer to a given key, creating a new group if it does not exist yet.
   /// Updates the 'result' and 'is_new_key' arguments to give the caller
   /// insight into whether this key was new.
   void lookupOrInsert(char** result, bool* is_new_key, const char* key);

   /// Get an iterator to the first non-empty element of the hash table.
   /// Sets it_data to nullptr if the iterator is exhausted.
   void iteratorStart(char** it_data, uint64_t* it_idx);
   /// Advance an iterator to the next non-empty element in the hash table.
   /// Sets it_data to nullptr if the iterator is exhausted.
   void iteratorAdvance(char** it_data, uint64_t* it_idx);

   /// Get the current size. Mainly used for testing.
   size_t size() const;
   /// Get the current capacity. Mainly used for testing.
   size_t capacity() const;

   size_t simpleKeySize() const { return simple_key_size; }
   size_t complexKeySlots() const { return complex_key_slots; }
   size_t payloadSize() const { return payload_size; }

   private:
   SharedHashTableState state;
   /// Size of the materialized simple key.
   uint16_t simple_key_size;
   /// Number of slots for complex keys, 8 bytes per slot.
   uint16_t complex_key_slots;
   /// Size of the payload in bytes.
   uint16_t payload_size;

   private:
   struct LookupResult {
      char* elem;
      uint8_t* tag;
   };

   /// Find the correct slot for the key, or the first one which is empty.
   inline LookupResult findSlotOrEmpty(uint64_t hash, const char* key);
   /// Find the first empty slot for the given hash.
   inline LookupResult findFirstEmptySlot(uint64_t hash);
   /// Make sure one more slot can be added to the hash table.
   /// If not, doubles size.
   void reserveSlot();
};

/// A hash table using direct lookup on the key index. No hashing, no nothing.
/// Should be used for 2 byte keys. Okay - this is a bit micro-optimized for TPC-H Q1,
/// we should also have a one byte variation. But we don't need it at the moment.
/// This is not an architectural limitation.
struct alignas(64) HashTableDirectLookup {
   /// Unique Hash Table ID.
   static const std::string ID;

   HashTableDirectLookup(uint16_t payload_size_);
   static size_t getMergeTableSize(
      std::deque<std::unique_ptr<HashTableDirectLookup>>& preagg, size_t thread_count);

   /// Get the pointer to a given key, or nullptr if the group does not exist.
   char* lookup(const char* key);
   /// Get the pointer to a given key, creating a new group if it does not exist yet.
   char* lookupOrInsert(const char* key);

   /// Compute the hash of some serialized key.
   uint64_t computeHash(const char* key) const;
   /// Get an iterator to the first non-empty element of the hash table.
   /// Sets it_data to nullptr if the iterator is exhausted.
   void iteratorStart(char** it_data, uint64_t* it_idx);
   /// Advance an iterator to the next non-empty element in the hash table.
   /// Sets it_data to nullptr if the iterator is exhausted.
   void iteratorAdvance(char** it_data, uint64_t* it_idx);
   /// Get the current size. Mainly used for testing.
   size_t size() const;
   /// Get the current capacity. Mainly used for testing.
   size_t capacity() const;

   /// Data managed by the hash table.
   std::unique_ptr<char[]> data;
   /// Tags indicating which slot contains data.
   std::unique_ptr<bool[]> tags;
   /// How many rows were inserted?
   size_t num_inserted = 0;
   /// Total slot size.
   uint16_t slot_size;
};
}

#endif //INKFUSE_HASHTABLES_H
