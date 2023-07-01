#ifndef INKFUSE_HASHTABLES_H
#define INKFUSE_HASHTABLES_H

#include <cstdint>
#include <memory>

/// This file contains the main hash tables used within InkFuse.
namespace inkfuse {

/// Shared state between the different hash table implementations.
struct SharedHashTableState {
   SharedHashTableState(uint16_t total_slot_size_, size_t start_slots_);

   /// Advance an iterator within the hash table.
   inline void advance(size_t& idx, char*& curr, uint8_t*& tag) const;
   /// Advance an iterator within the hash table. Sets the pointer to nullptr when the end of the hash table is reached.
   inline void advanceNoWrap(size_t& idx, char*& curr, uint8_t*& tag) const;

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
struct HashTableSimpleKey {
   /// Unique Hash Table ID.
   static const std::string ID;

   HashTableSimpleKey(uint16_t key_size_, uint16_t payload_size_, size_t start_slots_ = 2048);

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
struct HashTableComplexKey {
   /// Unique Hash Table ID.
   static const std::string ID;

   HashTableComplexKey(uint16_t simple_key_size, uint16_t complex_key_slots, uint16_t payload_size, size_t start_slots = 64);

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
struct HashTableDirectLookup {
   /// Unique Hash Table ID.
   static const std::string ID;

   HashTableDirectLookup(uint16_t payload_size_);

   /// Get the pointer to a given key, or nullptr if the group does not exist.
   char* lookup(const char* key);
   /// Get the pointer to a given key, creating a new group if it does not exist yet.
   char* lookupOrInsert(const char* key);


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
   /// Size of the materialized simple key.
   uint16_t key_size;
   /// Total slot size.
   uint16_t slot_size;
};

}

#endif //INKFUSE_HASHTABLES_H
