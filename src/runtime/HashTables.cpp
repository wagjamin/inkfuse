#include "runtime/HashTables.h"
#include "exec/ExecutionContext.h"
#include "xxhash.h"
#include <cassert>
#include <cstring>

namespace inkfuse {

namespace {
/// Highest order bit in the tag slot indicates if slot is filled.
const uint8_t tag_fill_mask = 1u << 7u;
/// Lower order 7 bits in the hash slot store salt of the hash.
const uint8_t tag_hash_mask = tag_fill_mask - 1;
}

SharedHashTableState::SharedHashTableState(uint16_t total_slot_size_, size_t start_slots_)
   : mod_mask(start_slots_ - 1), total_slot_size(total_slot_size_), max_fill(start_slots_ / 2) {
   if (start_slots_ < 2 || ((start_slots_ & (start_slots_ - 1)) != 0)) {
      throw std::runtime_error("Hash table start size has to power of 2 of at size 2.");
   }

   // Set up initial hash table based on the provided start size.
   // Allow half of the slots to be filled - this is needed to keep collision chains short.
   tags = std::make_unique<uint8_t[]>(start_slots_);
   data = std::make_unique<char[]>((start_slots_) *total_slot_size);
}

void SharedHashTableState::advance(size_t& idx, char*& data_ptr, uint8_t*& tag_ptr) const {
   assert(data_ptr != nullptr && tag_ptr != nullptr);
   if (idx == mod_mask) [[unlikely]] {
      // Wrap around.
      idx = 0;
      data_ptr = &data[0];
      tag_ptr = &tags[0];
   } else [[likely]] {
      // Regular advance.
      data_ptr += total_slot_size;
      tag_ptr++;
      idx++;
   }
}

void SharedHashTableState::advanceNoWrap(size_t& idx, char*& data_ptr, uint8_t*& tag_ptr) const {
   // Should not be called after it returned a nullptr.
   assert(data_ptr != nullptr && tag_ptr != nullptr);
   assert(idx <= mod_mask);
   if (idx == mod_mask) [[unlikely]] {
      // Reached end. Set the data to null.
      data_ptr = nullptr;
   } else [[likely]] {
      // Regular advance.
      data_ptr += total_slot_size;
      tag_ptr++;
      idx++;
   }
}

HashTableSimpleKey::HashTableSimpleKey(uint16_t key_size_, uint16_t payload_size_, size_t start_slots_)
   : state(key_size_ + payload_size_, start_slots_), simple_key_size(key_size_) {
   if (key_size_ == 0) {
      // If they key size is 0, we can always create a table with 2 slots, as at most
      // one will be filled.
      state = SharedHashTableState(key_size_ + payload_size_, 2);
   }
}

char* HashTableSimpleKey::lookup(const char* key) {
   // First step: hash the key.
   const uint64_t hash = XXH3_64bits(key, simple_key_size);
   // Find the slot which we belong to.
   const auto slot = findSlotOrEmpty(hash, key);
   // Only if the slot was tagged did we actually find the key.
   return *slot.tag ? slot.elem : nullptr;
}

char* HashTableSimpleKey::lookupOrInsert(const char* key) {
   char* result;
   bool is_new_key;
   lookupOrInsert(&result, &is_new_key, key);
   return result;
}

void HashTableSimpleKey::lookupOrInsert(char** result, bool* is_new_key, const char* key) {
   // Double the hash table if we don't have enough space.
   // Strictly speaking a bit too passive, as we might not need the
   // slot of the key already exists. But this is a border-case.
   reserveSlot();
   // First step: hash the key.
   const uint64_t hash = XXH3_64bits(key, simple_key_size);
   const auto slot = findSlotOrEmpty(hash, key);
   if (!(*slot.tag)) {
      // Initialize the slot.
      auto target_tag = static_cast<uint8_t>(hash >> 56ul);
      *slot.tag = tag_fill_mask | target_tag;
      // Copy over the key.
      std::memcpy(slot.elem, key, simple_key_size);
      state.inserted++;
      *is_new_key = true;
   } else {
      *is_new_key = false;
   }
   *result = slot.elem;
}

char* HashTableSimpleKey::insert(const char* key)
{
   reserveSlot();
   const uint64_t hash = XXH3_64bits(key, simple_key_size);

   // Access the base table at the right index.
   uint64_t idx = hash & state.mod_mask;
   char* elem_ptr = &state.data[idx * state.total_slot_size];
   uint8_t* tag_ptr = &state.tags[idx];

   // Find the first free slot.
   for (;;) {
      const uint8_t tag_fill = *tag_ptr & tag_fill_mask;
      const uint8_t tag_hash = *tag_ptr & tag_hash_mask;
      if (!tag_fill) {
         // We found the first free slot. Mark it as occupied.
         auto target_tag = static_cast<uint8_t>(hash >> 56ul);
         *tag_ptr = tag_fill_mask | target_tag;
         // Copy over the key.
         std::memcpy(elem_ptr, key, simple_key_size);
         state.inserted++;
         return elem_ptr;
      }
      state.advance(idx, elem_ptr, tag_ptr);
   }
}

void HashTableSimpleKey::iteratorStart(char** it_data, size_t* it_idx) {
   *it_idx = 0;
   *it_data = &state.data[0];
   uint8_t* tag_ptr = &state.tags[0];
   while (((*tag_ptr & tag_fill_mask) == 0) && *it_data != nullptr) {
      // Advance iterator to the first occupied slot.
      state.advanceNoWrap(*it_idx, *it_data, tag_ptr);
   }
}

void HashTableSimpleKey::iteratorAdvance(char** it_data, size_t* it_idx) {
   assert(*it_data != nullptr);
   uint8_t* tag_ptr = &state.tags[*it_idx];
   // Advance once to the next slot.
   state.advanceNoWrap(*it_idx, *it_data, tag_ptr);
   while (((*tag_ptr & tag_fill_mask) == 0) && *it_data != nullptr) {
      // Advance until a free slot was found.
      state.advanceNoWrap(*it_idx, *it_data, tag_ptr);
   }
}

size_t HashTableSimpleKey::size() {
   return state.inserted;
}

size_t HashTableSimpleKey::capacity() {
   return state.mod_mask + 1;
}

char* HashTableSimpleKey::lookupOrInsertSingleKey()
{
   // We always return the first slot.
   // We don't need any key checking whatsoever.
   char* elem_ptr = &state.data[0];
   uint8_t* tag_ptr = &state.tags[0];
   if (!*tag_ptr) {
      // First insert - tag the slot.
      *tag_ptr = tag_fill_mask;
      state.inserted++;
   }
   return elem_ptr;
}

HashTableSimpleKey::LookupResult HashTableSimpleKey::findSlotOrEmpty(uint64_t hash, const char* key) {
   // Access the base table at the right index.
   uint64_t idx = hash & state.mod_mask;
   char* elem_ptr = &state.data[idx * state.total_slot_size];
   uint8_t* tag_ptr = &state.tags[idx];
   // Get the tag from the hash, take the highest order bits as these
   // have the lowest risk of collision (lowest order bits are used to compute the slot).
   auto target_tag = tag_hash_mask & static_cast<uint8_t>(hash >> 56ul);
   for (;;) {
      const uint8_t tag_fill = *tag_ptr & tag_fill_mask;
      const uint8_t tag_hash = *tag_ptr & tag_hash_mask;
      if (!tag_fill || (tag_hash == target_tag && (std::memcmp(elem_ptr, key, simple_key_size) == 0))) {
         // We either found the key or an empty slot indicating the key does not exist.
         return {.elem = elem_ptr, .tag = tag_ptr};
      }
      state.advance(idx, elem_ptr, tag_ptr);
   }
}

HashTableSimpleKey::LookupResult HashTableSimpleKey::findFirstEmptySlot(uint64_t hash) {
   uint64_t idx = hash & state.mod_mask;
   char* elem_ptr = &state.data[idx * state.total_slot_size];
   uint8_t* tag_ptr = &state.tags[idx];

   while (*tag_ptr != 0) {
      state.advance(idx, elem_ptr, tag_ptr);
   }
   return {.elem = elem_ptr, .tag = tag_ptr};
}

void HashTableSimpleKey::reserveSlot() {
   if (state.inserted < state.max_fill) [[likely]] {
      return;
   }

   // We need to resize. Indicate that this happened to the currently installed execution context.
   // This will force the driver of the query to rerun the primitive if this used the interpreted path.
   bool* try_restart_flag = ExecutionContext::tryGetInstalledRestartFlag();
   if (try_restart_flag) {
      *try_restart_flag = true;
   }

   char* curr_slot = &state.data[0];
   uint8_t* curr_tag = &state.tags[0];
   size_t old_max_slot = state.mod_mask;
   // Double the size.
   SharedHashTableState new_state(state.total_slot_size, 2 * (state.mod_mask + 1));
   std::swap(new_state, state);
   for (uint64_t idx = 0; idx <= old_max_slot; ++idx) {
      if (*curr_tag) {
         // If it's set, insert hash value into new table. Upper bit does not matter, so don't have to zero it out.
         const uint64_t hash = XXH3_64bits(curr_slot, simple_key_size);
         const auto slot = findFirstEmptySlot(hash);
         // Move over the tag.
         *slot.tag = *curr_tag;
         // Move over the full payload.
         std::memcpy(slot.elem, curr_slot, state.total_slot_size);
      }
      curr_slot += state.total_slot_size;
      curr_tag++;
   }
   state.inserted = new_state.inserted;
}

}
