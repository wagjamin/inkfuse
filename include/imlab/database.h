// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------

#ifndef INCLUDE_IMLAB_DATABASE_H_
#define INCLUDE_IMLAB_DATABASE_H_

#include "imlab/infra/hash.h"
#include "imlab/schema.h"
#include "imlab/storage/relation.h"
#include "gen/tpcc.h"
#include <array>
#include <istream>

namespace imlab {

class Database {
   public:
   Database();

   template <tpcc::Relation>
   void Load(std::istream& in);
   // Load the TPCC data.
   // Call these methods with std::ifstreams to load your data.
   void LoadCustomer(std::istream& in);
   void LoadDistrict(std::istream& in);
   void LoadHistory(std::istream& in);
   void LoadItem(std::istream& in);
   void LoadNewOrder(std::istream& in);
   void LoadOrder(std::istream& in);
   void LoadOrderLine(std::istream& in);
   void LoadStock(std::istream& in);
   void LoadWarehouse(std::istream& in);

   // Place a new order.
   void NewOrder(
      Integer w_id,
      Integer d_id,
      Integer c_id,
      Integer items,
      std::array<Integer, 15>& supware,
      std::array<Integer, 15>& itemid,
      std::array<Integer, 15>& qty,
      Timestamp datetime);

   // Create a delivery
   void Delivery(
      Integer w_id,
      Integer o_carrier_id,
      Timestamp datetime);

   // Run an analytical query with STL datastructures
   Numeric<12, 2> AnalyticalQuerySTL();
   // Run an analytical query with a lazy hash table
   Numeric<12, 2> AnalyticalQueryLHT();

   // Returns the number of tuples in a relation.
   template <tpcc::Relation>
   size_t Size();

    table_warehouse warehouse;
    table_district district;
    table_customer customer;
    table_history history;
    table_neworder neworder;
    table_order order;
    table_orderline orderline;
    table_item item;
    table_stock stock;
};

} // namespace imlab

#endif // INCLUDE_IMLAB_DATABASE_H_
