// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------

#include "imlab/database.h"
#include "imlab/infra/error.h"
#include "imlab/infra/types.h"
#include "imlab/infra/hash_table.h"
#include <array>
#include <iostream>
#include <functional>
#include <unordered_map>

namespace imlab {

namespace {
/// Table names for TPCC. Might be more verbose than storing the schema directly with the enum as
/// keys, but our solution is more general and can be extended easier lated.
static std::unordered_map<tpcc::Relation, std::string> tableNames = {
   {tpcc::Relation::kWarehouse, "warehouse"},
   {tpcc::Relation::kDistrict, "district"},
   {tpcc::Relation::kCustomer, "customer"},
   {tpcc::Relation::kHistory, "history"},
   {tpcc::Relation::kNewOrder, "neworder"},
   {tpcc::Relation::kOrder, "order"},
   {tpcc::Relation::kOrderLine, "orderline"},
   {tpcc::Relation::kItem, "item"},
   {tpcc::Relation::kStock, "stock"}};

} // namespace

Database::Database() {
}

void Database::LoadWarehouse(std::istream& in) {
    warehouse.loadRows(in);
}

void Database::LoadDistrict(std::istream& in) {
   district.loadRows(in);
}

void Database::LoadCustomer(std::istream& in) {
    customer.loadRows(in);
}

void Database::LoadHistory(std::istream& in) {
    history.loadRows(in);
}

void Database::LoadNewOrder(std::istream& in) {
    neworder.loadRows(in);
}

void Database::LoadOrder(std::istream& in) {
    order.loadRows(in);
}

void Database::LoadOrderLine(std::istream& in) {
    orderline.loadRows(in);
}

void Database::LoadItem(std::istream& in) {
    item.loadRows(in);
}

void Database::LoadStock(std::istream& in) {
    stock.loadRows(in);
}

template <>
size_t Database::Size<tpcc::kCustomer>() {
    return customer.getSize();
}

template <>
size_t Database::Size<tpcc::kDistrict>() {
    return district.getSize();
}

template <>
size_t Database::Size<tpcc::kHistory>() {
    return history.getSize();
}

template <>
size_t Database::Size<tpcc::kItem>() {
    return item.getSize();
}

template <>
size_t Database::Size<tpcc::kNewOrder>() {
    return neworder.getSize();
}

template <>
size_t Database::Size<tpcc::kOrder>() {
    return order.getSize();
}

template <>
size_t Database::Size<tpcc::kOrderLine>() {
    return orderline.getSize();
}

template <>
size_t Database::Size<tpcc::kStock>() {
    return stock.getSize();
}

template <>
size_t Database::Size<tpcc::kWarehouse>() {
    return warehouse.getSize();
}

void Database::NewOrder(
   Integer w_id,
   Integer d_id,
   Integer c_id,
   Integer items,
   std::array<Integer, 15>& supware,
   std::array<Integer, 15>& itemid,
   std::array<Integer, 15>& qty,
   Timestamp datetime) {
    // Results being pulled in through queries.
    // What is up if there's no value here? Assume for now that this always returns.
    Numeric<4, 4> w_tax;
    Numeric<4, 4> c_discount;
    Numeric<4, 4> d_tax;
    Integer o_id{0};


    // w_id is the PK, so we know this will only be one row
    // select w_tax from warehouse w where w.w_id=w_id;
    {
        table_warehouse::PKType pk{w_id};

        auto tid = warehouse.Lookup(pk);
        auto tuple = warehouse.Read(tid);

        w_tax = std::get<7>(tuple);
    }

    // again operating on PK, so we know this will only be one row
    // select c_discount from customer c where c_w_id=w_id and c_d_id=d_id and c.c_id=c_id;
    {
        table_customer::PKType pk{w_id, d_id, d_id};

        auto tid = customer.Lookup(pk);
        auto tuple = customer.Read(tid);

        c_discount = std::get<15>(tuple);
    }

    // again operating on PK, so we know this will only be one row
    // select d_next_o_id as o_id,d_tax from district d where d_w_id=w_id and d.d_id=d_id;
    // update district set d_next_o_id=o_id+1 where d_w_id=w_id and district.d_id=d_id;
    {
        table_district::PKType pk{w_id, d_id};

        auto tid = district.Lookup(pk);
        auto tuple = district.Read(tid);

        o_id = std::get<10>(tuple);

        std::get<10>(tuple) = o_id + Integer{1};
        district.Update(tid, tuple);
    }

    // var integer all_local = 1;
    Integer all_local{1};

    // forsequence (index between 0 and items-1) {
    //    if (w_id<>supware[index])
    //         all_local=0;
    {
        const Integer const_one{1};
        for (Integer index{0}; index < items; index = index + const_one) {
            if (w_id != supware[index.value]) {
                all_local = Integer{0};
                // Early termination.
                break;
            }
        }
    }

    // insert into "order" values (o_id,d_id,w_id,c_id,datetime,0,items,all_local);
    {
        table_order::TupleType tuple{o_id, d_id, w_id, c_id, datetime, 0, items, all_local};
        order.Create(tuple);
    }

    // insert into neworder values (o_id,d_id,w_id);
    {
        table_neworder::TupleType tuple{o_id, d_id, w_id};
        neworder.Create(tuple);
    }

    /*
     forsequence (index between 0 and items-1) {
         select i_price from item where i_id=itemid[index];
         select s_quantity,s_remote_cnt,s_order_cnt,case d_id when 1 then s_dist_01 when 2 then s_dist_02 when 3 then s_dist_03 when 4 then s_dist_04 when 5 then s_dist_05 when 6 then s_dist_06 when 7 then s_dist_07 when 8 then s_dist_08 when 9 then s_dist_09 when 10 then s_dist_10 end as s_dist from stock where s_w_id=supware[index] and s_i_id=itemid[index];

         if (s_quantity>qty[index]) {
             update stock set s_quantity=s_quantity-qty[index] where s_w_id=supware[index] and s_i_id=itemid[index];
         } else {
             update stock set s_quantity=s_quantity+91-qty[index] where s_w_id=supware[index] and s_i_id=itemid[index];
         }

         if (supware[index]<>w_id) {
             update stock set s_remote_cnt=s_remote_cnt+1 where s_w_id=w_id and s_i_id=itemid[index];
         } else {
             update stock set s_order_cnt=s_order_cnt+1 where s_w_id=w_id and s_i_id=itemid[index];
         }

         var numeric(6,2) ol_amount=qty[index]*i_price*(1.0+w_tax+d_tax)*(1.0-c_discount);
         insert into orderline values (o_id,d_id,w_id,index+1,itemid[index],supware[index],0,qty[index],ol_amount,s_dist);
     }
     */
    {
        const Integer const_one{1};
        for (Integer index{0}; index < items; index = index + const_one) {

            Numeric<5, 2> i_price;
            Numeric<4, 0> s_quantity;
            Numeric<4, 0> s_remote_cnt;
            Numeric<4, 0> s_order_cnt;
            Char<24> s_dist;

            // select i_price from item where i_id=itemid[index];
            {
                table_item::PKType pk{itemid[index.value]};

                auto tid = item.Lookup(pk);
                auto tuple = item.Read(tid);

                i_price = std::get<3>(tuple);
            }


            {
                // select s_quantity,s_remote_cnt,s_order_cnt,case d_id when 1 then s_dist_01 when 2 then s_dist_02 when 3 then s_dist_03 when 4 then s_dist_04 when 5 then s_dist_05 when 6 then s_dist_06 when 7 then s_dist_07 when 8 then s_dist_08 when 9 then s_dist_09 when 10 then s_dist_10 end as s_dist from stock where s_w_id=supware[index] and s_i_id=itemid[index];
                table_stock::PKType pk{supware[index.value], itemid[index.value]};

                auto tid = stock.Lookup(pk);
                auto tuple = stock.Read(tid);

                s_quantity = std::get<2>(tuple);
                s_remote_cnt = std::get<15>(tuple);
                s_order_cnt = std::get<14>(tuple);

                // This is the only place where I severely move away from what a compiled engine would do.
                // Some domain knowledge really makes my life easier here :-)
                if (d_id == Integer{10}) {
                    s_dist = static_cast<TypedColumn<Char<24>>&>(stock.getColumn("s_dist_10")).getStorage()[tid];
                } else {
                    std::string stringified = std::to_string(d_id.value);
                    assert(stringified.length() == 1);
                    s_dist = static_cast<TypedColumn<Char<24>>&>(stock.getColumn("s_dist_0" + stringified)).getStorage()[tid];
                }

                if (s_quantity > Numeric<4, 0>{qty[index.value]}) {
                    // update stock set s_quantity=s_quantity-qty[index] where s_w_id=supware[index] and s_i_id=itemid[index];
                    std::get<2>(tuple) = s_quantity - Numeric<4, 0>{qty[index.value]};
                } else {
                    // update stock set s_quantity=s_quantity+91-qty[index] where s_w_id=supware[index] and s_i_id=itemid[index];
                    std::get<2>(tuple) = s_quantity + Numeric<4, 0>(91) - Numeric<4 ,0>{qty[index.value]};
                }

                stock.Update(tid, tuple);
            }

            {
                table_stock::PKType pk{w_id, itemid[index.value]};

                auto tid = stock.Lookup(pk);
                auto tuple = stock.Read(tid);

                if (supware[index.value] != w_id) {
                    // update stock set s_remote_cnt=s_remote_cnt+1 where s_w_id=w_id and s_i_id=itemid[index];
                    std::get<15>(tuple) = s_remote_cnt + Numeric<4,0>(1);
                } else {
                    // update stock set s_order_cnt=s_order_cnt+1 where s_w_id=w_id and s_i_id=itemid[index];
                    std::get<15>(tuple) = s_order_cnt + Numeric<4,0>(1);
                }

                stock.Update(tid, tuple);
            }

            // var numeric(6,2) ol_amount=qty[index]*i_price*(1.0+w_tax+d_tax)*(1.0-c_discount);
            // Increasing precision during the calculation and doing the final division in the end to retain exactness.
            Numeric<6, 2> ol_amount{
                (((Numeric<6, 2>(qty[index.value]) * Numeric<6, 2>(i_price.value))
                    * Numeric<6, 4>((Numeric<4, 4>(10000) - w_tax + d_tax).value))
                    * Numeric<6, 8>((Numeric<4, 4>(10000) - c_discount).value * 1000)).value / 1000};


            // insert into orderline values (o_id,d_id,w_id,index+1,itemid[index],supware[index],0,qty[index],ol_amount,s_dist);
            table_orderline::TupleType tuple {
                o_id,
                d_id,
                w_id,
                index + Integer{1},
                itemid[index.value],
                supware[index.value],
                0,
                qty[index.value].value * 100,
                ol_amount,
                s_dist
            };

            orderline.Create(tuple);

        }
    }
}

    void Database::Delivery(
            Integer w_id,
   Integer o_carrier_id,
   Timestamp datetime) {

    for (Integer d_id{1}; d_id <= Integer{10}; d_id += Integer{1}) {
        table_neworder::PKType start{w_id, d_id, 0};
        table_neworder::PKType end{w_id, d_id + Integer{1}, 0};
        auto it = table_neworder::KeyIterator(neworder, start, end);
        Integer o_id;
        // Select minimum over PK scan, we can use the fact that no_o_id is final PK field
        if (auto elem = it.next()) {
            o_id = std::get<0>(*elem);
        }
        else {
            continue;
        }

        table_neworder::PKType del_row{w_id, d_id, o_id};
        size_t del_tid = neworder.Lookup(del_row);
        neworder.Delete(del_tid);

        table_order::PKType lookup_row_order{w_id, d_id, o_id};
        size_t order_tid = order.Lookup(lookup_row_order);
        auto row = order.Read(order_tid);
        auto o_ol_cnt = std::get<6>(row);
        auto o_c_id = std::get<5>(row);
        // Update value.
        std::get<5>(row) = o_carrier_id;
        order.Update(order_tid, row);

        Numeric<6, 2> ol_total{0};
        for (auto ol_number = Integer{1}; ol_number.value <= o_ol_cnt.value; ol_number += Integer{1}) {
            // Select.
            table_orderline::PKType lookup_row_orderline{w_id, d_id, o_id, ol_number};
            auto orderline_tid = orderline.Lookup(lookup_row_orderline);
            auto orderline_row = orderline.Read(orderline_tid);
            auto ol_amount = std::get<8>(orderline_row);
            ol_total += ol_amount;
            std::get<6>(orderline_row) = datetime;
            // Update.
            orderline.Update(orderline_tid, orderline_row);
        }

        table_customer::PKType lookup_row_customer{w_id, d_id, o_c_id};
        auto customer_tid = customer.Lookup(lookup_row_customer);
        auto customer_row = customer.Read(customer_tid);
        std::get<16>(customer_row) += ol_total.castS<12>();
        customer.Update(customer_tid, customer_row);
    }

}

// Boost hash combine
template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

struct TupleHasher {
    std::size_t operator()(table_order::PKType const& val) const noexcept
    {
        std::size_t h1 = std::hash<Integer>{}(std::get<0>(val));
        hash_combine(h1, std::hash<Integer>{}(std::get<1>(val)));
        hash_combine(h1, std::hash<Integer>{}(std::get<2>(val)));
        return h1;
    }
};

Numeric<12, 2> Database::AnalyticalQuerySTL() {

    // Join order: customer -> (order -> orderline)
    Numeric<12, 2> result{0};

    // STEP 1: Build HT on orders
    std::unordered_map<table_order::PKType, table_order::TupleType, TupleHasher> ht_0;
    {
        // Prepare full table scan over orders.
        table_order::PKType start = {Integer{0}, Integer{0}, Integer{0}};
        table_order::PKType end = {INTEGER_MAX, INTEGER_MAX, INTEGER_MAX};
        auto it = table_order::KeyIterator(order, start, end);
        while (auto tuple = it.next()) {
            ht_0.emplace(table_order::Assemble(*tuple), *tuple);
        }
    }

    // STEP 2: Build HT on customer
    std::unordered_map<table_customer::PKType, table_customer::TupleType, TupleHasher> ht_1;
    {
        // Prepare full table scan over customers.
        table_customer::PKType start = {Integer{0}, Integer{0}, Integer{0}};
        table_customer::PKType end = {INTEGER_MAX, INTEGER_MAX, INTEGER_MAX};
        auto it = table_customer::KeyIterator(customer, start, end);
        while (auto tuple = it.next()) {
            // Filter pushdown.
            if (std::get<5>(*tuple).value[0] == 'B') {
                ht_1.emplace(table_customer::Assemble(*tuple), *tuple);
            }
        }
    }

    // STEP 3: Probe on oderline in a full pipeline
    // Prepare full table scan over customers.
    table_orderline::PKType start = {Integer{0}, Integer{0}, Integer{0}, Integer{0}};
    table_orderline::PKType end = {INTEGER_MAX, INTEGER_MAX, INTEGER_MAX, INTEGER_MAX};
    auto it = table_orderline::KeyIterator(orderline, start, end);
    while (auto tuple = it.next()) {
        // Probe order.
        table_order::PKType probe_ht_0{std::get<2>(*tuple), std::get<1>(*tuple), std::get<0>(*tuple)};
        auto elem_0 = ht_0.find(probe_ht_0);
        // Join 1, order left side
        if (elem_0 != ht_0.end()) {
            const auto& row_0 = elem_0->second;
            table_customer::PKType probe_ht_1{std::get<2>(row_0), std::get<1>(row_0), std::get<3>(row_0)};
            auto elem_1 = ht_1.find(probe_ht_1);
            // Join 2, customer left side
            if (elem_1 != ht_1.end()) {
                const auto& row_1 = elem_1->second;
                result += (std::get<7>(*tuple).castP2<12>() * std::get<8>(*tuple).castS<12>()).castM2<12>() - (std::get<16>(row_1) * std::get<6>(row_0).castP2<12>()).castM2<12>();
            }
        }
    }

    return result;
}

Numeric<12, 2> Database::AnalyticalQueryLHT() {
    // Join order: customer -> (order -> orderline)
    Numeric<12, 2> result{0};

    // STEP 1: Build HT on orders
    LazyMultiMap<table_order::PKType, table_order::TupleType, TupleHasher> ht_0;
    {
        // Prepare full table scan over orders.
        table_order::PKType start = {Integer{0}, Integer{0}, Integer{0}};
        table_order::PKType end = {INTEGER_MAX, INTEGER_MAX, INTEGER_MAX};
        auto it = table_order::KeyIterator(order, start, end);
        while (auto tuple = it.next()) {
            ht_0.insert(table_order::Assemble(*tuple), *tuple);
        }
        ht_0.finalize();
    }

    // STEP 2: Build HT on customer
    LazyMultiMap<table_customer::PKType, table_customer::TupleType, TupleHasher> ht_1;
    {
        // Prepare full table scan over customers.
        table_customer::PKType start = {Integer{0}, Integer{0}, Integer{0}};
        table_customer::PKType end = {INTEGER_MAX, INTEGER_MAX, INTEGER_MAX};
        auto it = table_customer::KeyIterator(customer, start, end);
        while (auto tuple = it.next()) {
            // Filter pushdown.
            if (std::get<5>(*tuple).value[0] == 'B') {
                ht_1.insert(table_customer::Assemble(*tuple), *tuple);
            }
        }
        ht_1.finalize();
    }

    // STEP 3: Probe on oderline in a full pipeline
    // Prepare full table scan over customers.
    table_orderline::PKType start = {Integer{0}, Integer{0}, Integer{0}, Integer{0}};
    table_orderline::PKType end = {INTEGER_MAX, INTEGER_MAX, INTEGER_MAX, INTEGER_MAX};
    auto it = table_orderline::KeyIterator(orderline, start, end);
    while (auto tuple = it.next()) {
        // Probe order.
        table_order::PKType probe_ht_0{std::get<2>(*tuple), std::get<1>(*tuple), std::get<0>(*tuple)};
        auto [ht_0_start, ht_0_end] = ht_0.equal_range(probe_ht_0);
        for (; ht_0_start != ht_0_end; ++ht_0_start) {
            // Join 1, order left side
            const auto& row_0 = *ht_0_start;
            table_customer::PKType probe_ht_1{std::get<2>(row_0), std::get<1>(row_0), std::get<3>(row_0)};
            auto [ht_1_start, ht_1_end] = ht_1.equal_range(probe_ht_1);
            for (; ht_1_start != ht_1_end; ++ht_1_start) {
                const auto& row_1 = *ht_1_start;
                result += (std::get<7>(*tuple).castP2<12>() * std::get<8>(*tuple).castS<12>()).castM2<12>() - (std::get<16>(row_1) * std::get<6>(row_0).castP2<12>()).castM2<12>();
            }
        }
    }

    return result;
}

} // namespace imlab
