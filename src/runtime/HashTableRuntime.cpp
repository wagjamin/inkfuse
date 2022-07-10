#include "runtime/HashTableRuntime.h"
#include "runtime/Runtime.h"
#include <cstring>

namespace inkfuse {

namespace {
// Lower order 63 bits in the hash slot store the hash suffix.
const uint64_t fill_mask = 1ull << 63ull;
// Highest order bit in the hash slot indicates if slot is filled.
const uint64_t hash_mask = fill_mask - 1;
}

extern "C" char* HashTableRuntime::ht_lookup(void* table, uint64_t hash) {
   return reinterpret_cast<HashTable*>(table)->lookup(hash);
}

extern "C" char* HashTableRuntime::ht_lookupNext(void* table, char* data) {
   return reinterpret_cast<HashTable*>(table)->lookupNext(data);
}

extern "C" char* HashTableRuntime::ht_insert(void* table, uint64_t hash) {
   return reinterpret_cast<HashTable*>(table)->insert(hash);
}

void HashTableRuntime::registerRuntime() {
   RuntimeFunctionBuilder("ht_lookup", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("hash", IR::UnsignedInt::build(8));

   RuntimeFunctionBuilder("ht_lookupNext", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("data", IR::Pointer::build(IR::Char::build()));

   RuntimeFunctionBuilder("ht_insert", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("hash", IR::UnsignedInt::build(8));
}

HashTable::HashTable(uint16_t payload_size_, size_t start_size)
   : slot_size(8 + payload_size_) {
   if (start_size < 4 || ((start_size & (start_size - 1)) != 0)) {
      throw std::runtime_error("Hash table start size has to power of 2 of at least 4.");
   }
   // Set up initial hash table based on the provided start size.
   // Allow half of the slots to be filled - this is probably too pessimistic.
   slots = start_size;
   max_fill = slots / 2;
   mod_mask = start_size - 1;
   data = std::make_unique<char[]>(slots * slot_size);
   last_index = &data[(slots - 1) * slot_size];
}

char* HashTable::advanceIndex(char* curr) {
   assert(curr != nullptr);
   return curr == last_index ? &data[0] : curr + slot_size;
}

char* HashTable::resolveChain(uint64_t hash, char* curr) {
   assert(curr != nullptr);
   uint64_t* key = reinterpret_cast<uint64_t*>(curr);
   // Resolve collision chain until hash is found or chain is exhausted.
   while ((*key & fill_mask) > 0 && (*key & hash_mask) != (hash & hash_mask)) {
      curr = advanceIndex(curr);
      key = reinterpret_cast<uint64_t*>(curr);
   }
   // Return nullptr if the chain is exhausted, otherwise start to payload.
   return (*key & fill_mask) == 0 ? nullptr : curr + 8;
}

char* HashTable::nextFreeSlot(char* curr) {
   assert(curr != nullptr);
   uint64_t* key = reinterpret_cast<uint64_t*>(curr);
   while ((*key & fill_mask) > 0) {
      curr = advanceIndex(curr);
      key = reinterpret_cast<uint64_t*>(curr);
   }
   return curr;
}

void HashTable::doubleSize() {
   // Allocate new hash table.
   const uint16_t payload_size = slot_size - 8;
   HashTable new_table(payload_size, 2 * slots);
   // Reinsert elements.
   char* curr = &data[0];
   for (uint64_t idx = 0; idx < slots; ++idx) {
      uint64_t* hash_slot = reinterpret_cast<uint64_t*>(curr);
      if ((*hash_slot & fill_mask) > 0) {
         // If it's set, insert hash value into new table. Upper bit does not matter, so don't have to zero it out.
         char* new_elem = new_table.insert(*hash_slot);
         // Copy over the payload.
         std::memcpy(new_elem, curr + 8, payload_size);
      }
      curr += slot_size;
   }
   // And move over into ourselves.
   *this = std::move(new_table);
}

char* HashTable::lookup(uint64_t hash) {
   // Compute lookup index.
   const uint64_t idx = hash & mod_mask;
   // And access the base table at the right index.
   char* elem = &data[slot_size * idx];
   return resolveChain(hash, elem);
}

char* HashTable::lookupNext(char* data) {
   assert(data != nullptr);
   // Figure out current hash value in the slot. Note that we don't have to zero the highest-order bit as it gets zeroed later.
   uint64_t hash = *reinterpret_cast<uint64_t*>(data - 8);
   // Move to the next item.
   char* elem = data + slot_size - 8;
   return resolveChain(hash, elem);
}

char* HashTable::insert(uint64_t hash) {
   if (inserted + 1 > max_fill) {
      // Double hash-table size and reinsert.
      doubleSize();
   }
   inserted++;
   // Compute lookup index.
   const uint64_t idx = hash & mod_mask;
   // And access the base table at the right index.
   char* elem = &data[slot_size * idx];
   // Resolve the next free slot.
   char* slot = nextFreeSlot(elem);
   // Mark as set and return.
   *reinterpret_cast<uint64_t*>(slot) = fill_mask | (hash & hash_mask);
   return slot + 8;
}

// Let's make sure that HashTable state nicely fits on a single cache line.
static_assert(sizeof(HashTable) <= 64);

}