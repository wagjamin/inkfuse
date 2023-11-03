#include "runtime/NewHashTables.h"
#include "xxhash.h"
#include <cassert>
#include <cstring>
#include <stdexcept>

namespace inkfuse {

namespace {
/// Highest order bit in the tag slot indicates if slot is filled.
const uint8_t tag_fill_mask = 1u << 7u;
/// Mask that allows inverting hash fingerprint but doesn't touch the fill bit.
const uint8_t fingerprint_inversion_mask = tag_fill_mask - 1;
/// Lower order 7 bits in the hash slot store salt of the hash.
const uint8_t tag_hash_mask = tag_fill_mask - 1;
}

template <>
const std::string AtomicHashTable<SimpleKeyComparator>::ID = "at_sk";
template <>
const std::string AtomicHashTable<ComplexKeyComparator>::ID = "at_ck";
template <>
const std::string ExclusiveHashTable<SimpleKeyComparator>::ID = "ex_sk";
template <>
const std::string ExclusiveHashTable<ComplexKeyComparator>::ID = "ex_ck";

SimpleKeyComparator::SimpleKeyComparator(uint16_t key_size_) : key_size(key_size_) {
}

bool SimpleKeyComparator::eq(const char* k1, const char* k2) const {
   return std::memcmp(k1, k2, key_size) == 0;
}

uint64_t SimpleKeyComparator::hash(const char* k) const {
   return XXH3_64bits(k, key_size);
}

uint16_t SimpleKeyComparator::keySize() const {
   return key_size;
}

ComplexKeyComparator::ComplexKeyComparator(uint16_t complex_key_slots_, uint16_t simple_key_size_)
   : complex_key_slots(complex_key_slots_), simple_key_size(simple_key_size_) {
   // TODO(benjamin) - Extend beyond a single string key. Pretty easy, but not important right now.
   if (simple_key_size != 0 || complex_key_slots != 1) {
      throw std::runtime_error("InkFuse currently only supports complex hash tables with a single string.");
   }
}

bool ComplexKeyComparator::eq(const char* k1, const char* k2) const {
   // k1 and k2 point at the first slot. We need to do string compare
   // on the pointer stored directly at that pointer.
   const char* indirection_k1 = *reinterpret_cast<const char* const*>(k1);
   const char* indirection_k2 = *reinterpret_cast<const char* const*>(k2);
   return std::strcmp(indirection_k1, indirection_k2) == 0;
}

uint16_t ComplexKeyComparator::keySize() const {
   return 8;
}

uint64_t ComplexKeyComparator::hash(const char* k) const {
   const auto indirection = reinterpret_cast<char* const*>(k);
   const size_t len = std::strlen(*indirection);
   return XXH3_64bits(*indirection, len);
}

template <class Comparator>
AtomicHashTable<Comparator>::AtomicHashTable(Comparator comp_, uint16_t total_slot_size_, size_t num_slots_)
   : comp(comp_),
     total_slot_size(total_slot_size_),
     num_slots(num_slots_),
     mod_mask(num_slots_ - 1) {
   if (num_slots < 2 || ((num_slots & (num_slots - 1)) != 0)) {
      throw std::runtime_error(
         "Atomic hash table start size has to power of 2 of at least size 2. Provided " +
         std::to_string(num_slots));
   }

   // Set up hash table based on the provided start size.
   tags = std::make_unique<std::atomic<uint8_t>[]>(num_slots);
   data = std::make_unique<char[]>(num_slots * total_slot_size);
}

template <class Comparator>
uint64_t AtomicHashTable<Comparator>::compute_hash_and_prefetch(const char* key) const {
   uint64_t hash = comp.hash(key);
   const uint64_t slot_id = hash & mod_mask;
   // Prefetch the actual data array.
   __builtin_prefetch(&data[slot_id * total_slot_size]);
   // Prefetch the bitmask slot.
   __builtin_prefetch(&tags[slot_id]);
   return hash;
}

template <class Comparator>
void AtomicHashTable<Comparator>::slot_prefetch(uint64_t hash) const {
   const uint64_t slot_id = hash & mod_mask;
   // Prefetch the actual data array.
   __builtin_prefetch(&data[slot_id * total_slot_size]);
   // Prefetch the bitmask slot.
   __builtin_prefetch(&tags[slot_id]);
}

template <class Comparator>
char* AtomicHashTable<Comparator>::lookup(const char* key, uint64_t hash) const {
   const uint64_t slot_id = hash & mod_mask;
   // Look up the initial slot in the linear probing chain.
   IteratorState it{
      .idx = slot_id,
      .data_ptr = &data[slot_id * total_slot_size],
      .tag_ptr = &tags[slot_id],
   };
   // The tag we are looking for.
   const uint8_t target_tag = tag_fill_mask | static_cast<uint8_t>(hash >> 56ul);
   for (;;) {
      const uint8_t curr_tag = it.tag_ptr->load();
      if (!(curr_tag & tag_fill_mask)) {
         // End of the linear probing chain - the key does not exist.
         return nullptr;
      }
      if (curr_tag == target_tag && comp.eq(key, it.data_ptr)) {
         // Same tag and key, we found the value.
         return it.data_ptr;
      }
      // Different tag in the chain, move on to the next key.
      itAdvance(it);
   }
   return it.data_ptr;
}

template <class Comparator>
char* AtomicHashTable<Comparator>::lookup(const char* key) const {
   const uint64_t hash = comp.hash(key);
   return lookup(key, hash);
}

template <class Comparator>
char* AtomicHashTable<Comparator>::lookupDisable(const char* key, uint64_t hash) {
   // Look up the initial slot in the linear probing chain.
   const auto idx = hash & mod_mask;
   IteratorState it{
      .idx = idx,
      .data_ptr = &data[idx * total_slot_size],
      .tag_ptr = &tags[idx],
   };
   // The tag we are looking for.
   const uint8_t target_tag = tag_fill_mask | static_cast<uint8_t>(hash >> 56ul);
   // Tag for the disabled slot.
   // What actually happens here is a bit of bit fiddling: we keep the slot enabled but invert
   // the lower 7 bit (hash fingerprint).
   // This way, any future lookup on the key will not find this tag anymore.
   // As a result, the row won't be found. At the same time, the chain stays intact.
   const uint8_t inverted_tag = target_tag ^ fingerprint_inversion_mask;
   for (;;) {
      const uint8_t curr_tag = it.tag_ptr->load();
      if (!(curr_tag & tag_fill_mask)) {
         // End of the linear probing chain - the key does not exist.
         return nullptr;
      }
      if (curr_tag == target_tag && comp.eq(key, it.data_ptr)) {
         uint8_t expected = target_tag;
         if (it.tag_ptr->compare_exchange_strong(expected, inverted_tag)) {
            // Same tag and key, we found the value. Disabling worked as well, we are done.
            return it.data_ptr;
         }
         // Another thread disabled the slot at the same time, we need to move on.
      }
      // Different tag in the chain, move on to the next key.
      itAdvance(it);
   }
   return it.data_ptr;
}

template <class Comparator>
char* AtomicHashTable<Comparator>::lookupDisable(const char* key) {
   const uint64_t hash = comp.hash(key);
   return lookupDisable(key, hash);
}

template <class Comparator>
template <bool copy_only_key>
char* AtomicHashTable<Comparator>::insert(const char* key, uint64_t hash) {
   // Look up the initial slot in the linear probing chain .
   const auto idx = hash & mod_mask;
   IteratorState it{
      .idx = idx,
      .data_ptr = &data[idx * total_slot_size],
      .tag_ptr = &tags[idx],
   };
   // Find the first free slot and mark it.
   const uint8_t target_tag = tag_fill_mask | static_cast<uint8_t>(hash >> 56ul);
   for (;;) {
      uint8_t curr_tag = it.tag_ptr->load();
      const uint8_t tag_fill = curr_tag & tag_fill_mask;
      if (!tag_fill && it.tag_ptr->compare_exchange_strong(curr_tag, target_tag)) {
         // We managed to claim the slot. Populate it.
         if constexpr (copy_only_key) {
            std::memcpy(it.data_ptr, key, comp.keySize());
         } else {
            std::memcpy(it.data_ptr, key, total_slot_size);
         }
         return it.data_ptr;
      }
      // Either the slot was already tagged, or got tagged by another thread at the same time.
      // Move on to the next tag and try again.
      itAdvance(it);
   }
}

template <class Comparator>
template <bool copy_only_key>
char* AtomicHashTable<Comparator>::insert(const char* key) {
   const uint64_t hash = comp.hash(key);
   return insert(key, hash);
}

template <class Comparator>
typename AtomicHashTable<Comparator>::IteratorState AtomicHashTable<Comparator>::itStart() const {
   IteratorState it;
   it.idx = 0;
   it.data_ptr = &data[0];
   auto tag_ptr = &tags[0];
   while (((tag_ptr->load() & tag_fill_mask) == 0) && it.data_ptr != nullptr) {
      // Advance iterator to the first occupied slot or until the end.
      itAdvanceNoWrap(it);
   }
   return it;
}

template <class Comparator>
void AtomicHashTable<Comparator>::itAdvance(IteratorState& it) const {
   assert(it.data_ptr != nullptr && it.tag_ptr != nullptr);
   if (it.idx == mod_mask) [[unlikely]] {
         // Wrap around.
         it.idx = 0;
         it.data_ptr = &data[0];
         it.tag_ptr = &tags[0];
      }
   else
      [[likely]] {
         // Regular advance.
         it.data_ptr += total_slot_size;
         it.tag_ptr++;
         it.idx++;
      }
}

template <class Comparator>
void AtomicHashTable<Comparator>::itAdvanceNoWrap(IteratorState& it) const {
   // Should not be called after it returned a nullptr.
   assert(it.data_ptr != nullptr && it.tag_ptr != nullptr);
   assert(it.idx <= mod_mask);
   if (it.idx == mod_mask) [[unlikely]] {
         // Reached end. Set the data to null.
         it.data_ptr = nullptr;
      }
   else
      [[likely]] {
         // Regular advance.
         it.data_ptr += total_slot_size;
         it.tag_ptr++;
         it.idx++;
      }
}

// Declare all permitted template instantiations.
template class AtomicHashTable<SimpleKeyComparator>;
template char* AtomicHashTable<SimpleKeyComparator>::insert<true>(const char* key);
template char* AtomicHashTable<SimpleKeyComparator>::insert<false>(const char* key);

template char* AtomicHashTable<SimpleKeyComparator>::insert<true>(const char* key, uint64_t hash);
template char* AtomicHashTable<SimpleKeyComparator>::insert<false>(const char* key, uint64_t hash);

template class AtomicHashTable<ComplexKeyComparator>;
template char* AtomicHashTable<ComplexKeyComparator>::insert<true>(const char* key);
template char* AtomicHashTable<ComplexKeyComparator>::insert<false>(const char* key);

template char* AtomicHashTable<ComplexKeyComparator>::insert<true>(const char* key, uint64_t hash);
template char* AtomicHashTable<ComplexKeyComparator>::insert<false>(const char* key, uint64_t hash);

template class ExclusiveHashTable<SimpleKeyComparator>;
template class ExclusiveHashTable<ComplexKeyComparator>;

};
