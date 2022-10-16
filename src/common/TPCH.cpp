#include "common/TPCH.h"

// We are not 100% standard TPC-H schema compliant.
// - DECIMALs gets represented as double
// - VARCHARs/CHARs get represented as TEXT
//   (only exception is char(1) which gets represented as char)
namespace inkfuse::tpch {

namespace {

const auto t_ui4 = IR::UnsignedInt::build(4);
const auto t_f8 = IR::Float::build(8);
const auto t_char = IR::Float::build(8);
const auto t_date = IR::Float::build(8);

StoredRelationPtr tableLineitem() {
   auto rel = std::make_unique<StoredRelation>();
   rel->attachPODColumn("l_orderkey", t_ui4);
   rel->attachPODColumn("l_partkey", t_ui4);
   rel->attachPODColumn("l_suppkey", t_ui4);
   rel->attachPODColumn("l_linenumber", t_ui4);
   rel->attachPODColumn("l_quantity", t_f8);
   rel->attachPODColumn("l_extendedprice", t_f8);
   rel->attachPODColumn("l_discount", t_f8);
   rel->attachPODColumn("l_tax", t_f8);
   rel->attachPODColumn("l_returnflag", t_char);
   rel->attachPODColumn("l_linestatus", t_char);
   rel->attachPODColumn("l_shipdate", t_date);
   rel->attachPODColumn("l_commitdate", t_date);
   rel->attachPODColumn("l_receiptdate", t_date);
   rel->attachStringColumn("l_shipinstruct");
   rel->attachStringColumn("l_shipmode");
   rel->attachStringColumn("l_comment");
   return rel;
}

}

Schema getTPCHSchema() {
   Schema schema;
   schema.emplace("lineitem", tableLineitem());
   return schema;
}

}