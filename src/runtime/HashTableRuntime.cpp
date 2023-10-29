#include "runtime/HashTableRuntime.h"
#include "runtime/HashTables.h"
#include "runtime/NewHashTables.h"
#include "runtime/Runtime.h"

namespace inkfuse {

void HashTableRuntime::registerRuntime() {
   RuntimeFunctionBuilder("ht_sk_insert", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_sk_lookup", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()), true)
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_sk_lookup_disable", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_sk_lookup_or_insert", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_sk_lookup_or_insert_with_init", IR::Void::build())
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("result", IR::Pointer::build(IR::Pointer::build(IR::Char::build())))
      .addArg("is_new_key", IR::Pointer::build(IR::Bool::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_sk_it_advance", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()), true)
      .addArg("it_data", IR::Pointer::build(IR::Pointer::build(IR::Char::build())))
      .addArg("it_idx", IR::Pointer::build(IR::UnsignedInt::build(8)));

   RuntimeFunctionBuilder("ht_nk_lookup", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()), true);

   RuntimeFunctionBuilder("ht_ck_lookup", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()), true)
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_ck_lookup_or_insert", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_ck_it_advance", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()), true)
      .addArg("it_data", IR::Pointer::build(IR::Pointer::build(IR::Char::build())))
      .addArg("it_idx", IR::Pointer::build(IR::UnsignedInt::build(8)));

   RuntimeFunctionBuilder("ht_dl_lookup", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()), true)
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_dl_lookup_or_insert", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_dl_it_advance", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()), true)
      .addArg("it_data", IR::Pointer::build(IR::Pointer::build(IR::Char::build())))
      .addArg("it_idx", IR::Pointer::build(IR::UnsignedInt::build(8)));

   // Atomic hash table.
   RuntimeFunctionBuilder("ht_at_sk_lookup", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_at_sk_lookup_disable", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_at_ck_lookup", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_at_sk_compute_hash_and_prefetch", IR::UnsignedInt::build(8))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_at_sk_slot_prefetch", IR::Void::build())
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("hash", IR::UnsignedInt::build(8), true);

   RuntimeFunctionBuilder("ht_at_sk_lookup_with_hash", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()))
      .addArg("hash", IR::UnsignedInt::build(8), true);

   RuntimeFunctionBuilder("ht_at_sk_lookup_with_hash_disable", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()))
      .addArg("hash", IR::UnsignedInt::build(8), true);

   RuntimeFunctionBuilder("ht_at_ck_compute_hash_and_prefetch", IR::UnsignedInt::build(8))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()), true);

   RuntimeFunctionBuilder("ht_at_ck_slot_prefetch", IR::Void::build())
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("hash", IR::UnsignedInt::build(8), true);

   RuntimeFunctionBuilder("ht_at_ck_lookup_with_hash", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()))
      .addArg("hash", IR::UnsignedInt::build(8), true);

   RuntimeFunctionBuilder("ht_at_ck_lookup_with_hash_disable", IR::Pointer::build(IR::Char::build()))
      .addArg("table", IR::Pointer::build(IR::Void::build()))
      .addArg("key", IR::Pointer::build(IR::Char::build()))
      .addArg("hash", IR::UnsignedInt::build(8), true);
}

}  // namespace inkfuse
