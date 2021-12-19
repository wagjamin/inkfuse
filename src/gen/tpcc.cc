#include "gen/tpcc.h"
namespace imlab {
table_warehouse::table_warehouse() {
    attachTypedColumn<Integer>("w_id");
    attachTypedColumn<Varchar<10>>("w_name");
    attachTypedColumn<Varchar<20>>("w_street_1");
    attachTypedColumn<Varchar<20>>("w_street_2");
    attachTypedColumn<Varchar<20>>("w_city");
    attachTypedColumn<Char<2>>("w_state");
    attachTypedColumn<Char<9>>("w_zip");
    attachTypedColumn<Numeric<4,4>>("w_tax");
    attachTypedColumn<Numeric<12,2>>("w_ytd");
}

table_district::table_district() {
    attachTypedColumn<Integer>("d_id");
    attachTypedColumn<Integer>("d_w_id");
    attachTypedColumn<Varchar<10>>("d_name");
    attachTypedColumn<Varchar<20>>("d_street_1");
    attachTypedColumn<Varchar<20>>("d_street_2");
    attachTypedColumn<Varchar<20>>("d_city");
    attachTypedColumn<Char<2>>("d_state");
    attachTypedColumn<Char<9>>("d_zip");
    attachTypedColumn<Numeric<4,4>>("d_tax");
    attachTypedColumn<Numeric<12,2>>("d_ytd");
    attachTypedColumn<Integer>("d_next_o_id");
}

table_customer::table_customer() {
    attachTypedColumn<Integer>("c_id");
    attachTypedColumn<Integer>("c_d_id");
    attachTypedColumn<Integer>("c_w_id");
    attachTypedColumn<Varchar<16>>("c_first");
    attachTypedColumn<Char<2>>("c_middle");
    attachTypedColumn<Varchar<16>>("c_last");
    attachTypedColumn<Varchar<20>>("c_street_1");
    attachTypedColumn<Varchar<20>>("c_street_2");
    attachTypedColumn<Varchar<20>>("c_city");
    attachTypedColumn<Char<2>>("c_state");
    attachTypedColumn<Char<9>>("c_zip");
    attachTypedColumn<Char<16>>("c_phone");
    attachTypedColumn<Timestamp>("c_since");
    attachTypedColumn<Char<2>>("c_credit");
    attachTypedColumn<Numeric<12,2>>("c_credit_lim");
    attachTypedColumn<Numeric<4,4>>("c_discount");
    attachTypedColumn<Numeric<12,2>>("c_balance");
    attachTypedColumn<Numeric<12,2>>("c_ytd_paymenr");
    attachTypedColumn<Numeric<4,0>>("c_payment_cnt");
    attachTypedColumn<Numeric<4,0>>("c_delivery_cnt");
    attachTypedColumn<Varchar<500>>("c_data");
}

table_history::table_history() {
    attachTypedColumn<Integer>("h_c_id");
    attachTypedColumn<Integer>("h_c_d_id");
    attachTypedColumn<Integer>("h_c_w_id");
    attachTypedColumn<Integer>("h_d_id");
    attachTypedColumn<Integer>("h_w_id");
    attachTypedColumn<Timestamp>("h_date");
    attachTypedColumn<Numeric<6,2>>("h_amount");
    attachTypedColumn<Varchar<24>>("h_data");
}

table_neworder::table_neworder() {
    attachTypedColumn<Integer>("no_o_id");
    attachTypedColumn<Integer>("no_d_id");
    attachTypedColumn<Integer>("no_w_id");
}

table_order::table_order() {
    attachTypedColumn<Integer>("o_id");
    attachTypedColumn<Integer>("o_d_id");
    attachTypedColumn<Integer>("o_w_id");
    attachTypedColumn<Integer>("o_c_id");
    attachTypedColumn<Timestamp>("o_entry_d");
    attachTypedColumn<Integer>("o_carrier_id");
    attachTypedColumn<Numeric<2,0>>("o_ol_cnt");
    attachTypedColumn<Numeric<1,0>>("o_all_local");
}

table_orderline::table_orderline() {
    attachTypedColumn<Integer>("ol_o_id");
    attachTypedColumn<Integer>("ol_d_id");
    attachTypedColumn<Integer>("ol_w_id");
    attachTypedColumn<Integer>("ol_number");
    attachTypedColumn<Integer>("ol_i_id");
    attachTypedColumn<Integer>("ol_supply_w_id");
    attachTypedColumn<Timestamp>("ol_delivery_d");
    attachTypedColumn<Numeric<2,0>>("ol_quantity");
    attachTypedColumn<Numeric<6,2>>("ol_amount");
    attachTypedColumn<Char<24>>("ol_dist_info");
}

table_item::table_item() {
    attachTypedColumn<Integer>("i_id");
    attachTypedColumn<Integer>("i_im_id");
    attachTypedColumn<Varchar<24>>("i_name");
    attachTypedColumn<Numeric<5,2>>("i_price");
    attachTypedColumn<Varchar<50>>("i_data");
}

table_stock::table_stock() {
    attachTypedColumn<Integer>("s_i_id");
    attachTypedColumn<Integer>("s_w_id");
    attachTypedColumn<Numeric<4,0>>("s_quantity");
    attachTypedColumn<Char<24>>("s_dist_01");
    attachTypedColumn<Char<24>>("s_dist_02");
    attachTypedColumn<Char<24>>("s_dist_03");
    attachTypedColumn<Char<24>>("s_dist_04");
    attachTypedColumn<Char<24>>("s_dist_05");
    attachTypedColumn<Char<24>>("s_dist_06");
    attachTypedColumn<Char<24>>("s_dist_07");
    attachTypedColumn<Char<24>>("s_dist_08");
    attachTypedColumn<Char<24>>("s_dist_09");
    attachTypedColumn<Char<24>>("s_dist_10");
    attachTypedColumn<Numeric<8,0>>("s_ytd");
    attachTypedColumn<Numeric<4,0>>("s_order_cnt");
    attachTypedColumn<Numeric<4,0>>("s_remote_cnt");
    attachTypedColumn<Varchar<50>>("s_data");
}

void table_warehouse::Create(
    const TupleType& newRow
) {
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        col.getStorage().push_back(std::get<0>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<10>>&>(getColumn(1).second);
        col.getStorage().push_back(std::get<1>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(2).second);
        col.getStorage().push_back(std::get<2>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(3).second);
        col.getStorage().push_back(std::get<3>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(4).second);
        col.getStorage().push_back(std::get<4>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<2>>&>(getColumn(5).second);
        col.getStorage().push_back(std::get<5>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<9>>&>(getColumn(6).second);
        col.getStorage().push_back(std::get<6>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,4>>&>(getColumn(7).second);
        col.getStorage().push_back(std::get<7>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<12,2>>&>(getColumn(8).second);
        col.getStorage().push_back(std::get<8>(newRow));
    }
    auto pk = Assemble(newRow);
    auto tid = getSize() - 1;
    pk_map[pk] = tid;
    appendRow();
}

table_warehouse::TupleType table_warehouse::Read(
    size_t tid
) {
    TupleType result;
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        std::get<0>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<10>>&>(getColumn(1).second);
        std::get<1>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(2).second);
        std::get<2>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(3).second);
        std::get<3>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(4).second);
        std::get<4>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Char<2>>&>(getColumn(5).second);
        std::get<5>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Char<9>>&>(getColumn(6).second);
        std::get<6>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,4>>&>(getColumn(7).second);
        std::get<7>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<12,2>>&>(getColumn(8).second);
        std::get<8>(result) = col.getStorage()[tid];
    }
    return result;
}

void table_warehouse::Update(
size_t tid,
    const TupleType& newRow
) {
    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map.erase(pk);
    auto pk_new = Assemble(newRow);
    pk_map[pk_new] = tid;
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        col.getStorage()[tid] = (std::get<0>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<10>>&>(getColumn(1).second);
        col.getStorage()[tid] = (std::get<1>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(2).second);
        col.getStorage()[tid] = (std::get<2>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(3).second);
        col.getStorage()[tid] = (std::get<3>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(4).second);
        col.getStorage()[tid] = (std::get<4>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<2>>&>(getColumn(5).second);
        col.getStorage()[tid] = (std::get<5>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<9>>&>(getColumn(6).second);
        col.getStorage()[tid] = (std::get<6>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,4>>&>(getColumn(7).second);
        col.getStorage()[tid] = (std::get<7>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<12,2>>&>(getColumn(8).second);
        col.getStorage()[tid] = (std::get<8>(newRow));
    }
}

void table_warehouse::Delete(
    size_t tid
) {
    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map.erase(pk);
    dropRow(tid);
}

void table_warehouse::addLastRowToIndex(
) {
    auto tid = getSize() - 1;    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map[pk] = tid;
}

size_t table_warehouse::Lookup(
    const PKType& key
) {
    return pk_map[key];
}

table_warehouse::PKType table_warehouse::Assemble(
    TupleType row
) {
    PKType result;
    std::get<0>(result) = std::get<0>(row);
    return result;
}

std::optional<table_warehouse::TupleType> table_warehouse::KeyIterator::next() {
    if (it_start != it_end) {
        TupleType result;
        size_t tid = it_start->second;
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(0).second);
            std::get<0>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Varchar<10>>&>(table.getColumn(1).second);
            std::get<1>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Varchar<20>>&>(table.getColumn(2).second);
            std::get<2>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Varchar<20>>&>(table.getColumn(3).second);
            std::get<3>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Varchar<20>>&>(table.getColumn(4).second);
            std::get<4>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Char<2>>&>(table.getColumn(5).second);
            std::get<5>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Char<9>>&>(table.getColumn(6).second);
            std::get<6>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Numeric<4,4>>&>(table.getColumn(7).second);
            std::get<7>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Numeric<12,2>>&>(table.getColumn(8).second);
            std::get<8>(result) = col.getStorage()[tid];
        }
        it_start++;
        return result;
    }
    return std::nullopt;
}

void table_district::Create(
    const TupleType& newRow
) {
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        col.getStorage().push_back(std::get<0>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        col.getStorage().push_back(std::get<1>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<10>>&>(getColumn(2).second);
        col.getStorage().push_back(std::get<2>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(3).second);
        col.getStorage().push_back(std::get<3>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(4).second);
        col.getStorage().push_back(std::get<4>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(5).second);
        col.getStorage().push_back(std::get<5>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<2>>&>(getColumn(6).second);
        col.getStorage().push_back(std::get<6>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<9>>&>(getColumn(7).second);
        col.getStorage().push_back(std::get<7>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,4>>&>(getColumn(8).second);
        col.getStorage().push_back(std::get<8>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<12,2>>&>(getColumn(9).second);
        col.getStorage().push_back(std::get<9>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(10).second);
        col.getStorage().push_back(std::get<10>(newRow));
    }
    auto pk = Assemble(newRow);
    auto tid = getSize() - 1;
    pk_map[pk] = tid;
    appendRow();
}

table_district::TupleType table_district::Read(
    size_t tid
) {
    TupleType result;
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        std::get<0>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        std::get<1>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<10>>&>(getColumn(2).second);
        std::get<2>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(3).second);
        std::get<3>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(4).second);
        std::get<4>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(5).second);
        std::get<5>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Char<2>>&>(getColumn(6).second);
        std::get<6>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Char<9>>&>(getColumn(7).second);
        std::get<7>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,4>>&>(getColumn(8).second);
        std::get<8>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<12,2>>&>(getColumn(9).second);
        std::get<9>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(10).second);
        std::get<10>(result) = col.getStorage()[tid];
    }
    return result;
}

void table_district::Update(
size_t tid,
    const TupleType& newRow
) {
    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map.erase(pk);
    auto pk_new = Assemble(newRow);
    pk_map[pk_new] = tid;
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        col.getStorage()[tid] = (std::get<0>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        col.getStorage()[tid] = (std::get<1>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<10>>&>(getColumn(2).second);
        col.getStorage()[tid] = (std::get<2>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(3).second);
        col.getStorage()[tid] = (std::get<3>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(4).second);
        col.getStorage()[tid] = (std::get<4>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(5).second);
        col.getStorage()[tid] = (std::get<5>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<2>>&>(getColumn(6).second);
        col.getStorage()[tid] = (std::get<6>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<9>>&>(getColumn(7).second);
        col.getStorage()[tid] = (std::get<7>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,4>>&>(getColumn(8).second);
        col.getStorage()[tid] = (std::get<8>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<12,2>>&>(getColumn(9).second);
        col.getStorage()[tid] = (std::get<9>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(10).second);
        col.getStorage()[tid] = (std::get<10>(newRow));
    }
}

void table_district::Delete(
    size_t tid
) {
    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map.erase(pk);
    dropRow(tid);
}

void table_district::addLastRowToIndex(
) {
    auto tid = getSize() - 1;    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map[pk] = tid;
}

size_t table_district::Lookup(
    const PKType& key
) {
    return pk_map[key];
}

table_district::PKType table_district::Assemble(
    TupleType row
) {
    PKType result;
    std::get<0>(result) = std::get<1>(row);
    std::get<1>(result) = std::get<0>(row);
    return result;
}

std::optional<table_district::TupleType> table_district::KeyIterator::next() {
    if (it_start != it_end) {
        TupleType result;
        size_t tid = it_start->second;
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(0).second);
            std::get<0>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(1).second);
            std::get<1>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Varchar<10>>&>(table.getColumn(2).second);
            std::get<2>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Varchar<20>>&>(table.getColumn(3).second);
            std::get<3>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Varchar<20>>&>(table.getColumn(4).second);
            std::get<4>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Varchar<20>>&>(table.getColumn(5).second);
            std::get<5>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Char<2>>&>(table.getColumn(6).second);
            std::get<6>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Char<9>>&>(table.getColumn(7).second);
            std::get<7>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Numeric<4,4>>&>(table.getColumn(8).second);
            std::get<8>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Numeric<12,2>>&>(table.getColumn(9).second);
            std::get<9>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(10).second);
            std::get<10>(result) = col.getStorage()[tid];
        }
        it_start++;
        return result;
    }
    return std::nullopt;
}

void table_customer::Create(
    const TupleType& newRow
) {
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        col.getStorage().push_back(std::get<0>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        col.getStorage().push_back(std::get<1>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(2).second);
        col.getStorage().push_back(std::get<2>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<16>>&>(getColumn(3).second);
        col.getStorage().push_back(std::get<3>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<2>>&>(getColumn(4).second);
        col.getStorage().push_back(std::get<4>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<16>>&>(getColumn(5).second);
        col.getStorage().push_back(std::get<5>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(6).second);
        col.getStorage().push_back(std::get<6>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(7).second);
        col.getStorage().push_back(std::get<7>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(8).second);
        col.getStorage().push_back(std::get<8>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<2>>&>(getColumn(9).second);
        col.getStorage().push_back(std::get<9>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<9>>&>(getColumn(10).second);
        col.getStorage().push_back(std::get<10>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<16>>&>(getColumn(11).second);
        col.getStorage().push_back(std::get<11>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Timestamp>&>(getColumn(12).second);
        col.getStorage().push_back(std::get<12>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<2>>&>(getColumn(13).second);
        col.getStorage().push_back(std::get<13>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<12,2>>&>(getColumn(14).second);
        col.getStorage().push_back(std::get<14>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,4>>&>(getColumn(15).second);
        col.getStorage().push_back(std::get<15>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<12,2>>&>(getColumn(16).second);
        col.getStorage().push_back(std::get<16>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<12,2>>&>(getColumn(17).second);
        col.getStorage().push_back(std::get<17>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,0>>&>(getColumn(18).second);
        col.getStorage().push_back(std::get<18>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,0>>&>(getColumn(19).second);
        col.getStorage().push_back(std::get<19>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<500>>&>(getColumn(20).second);
        col.getStorage().push_back(std::get<20>(newRow));
    }
    auto pk = Assemble(newRow);
    auto tid = getSize() - 1;
    pk_map[pk] = tid;
    appendRow();
}

table_customer::TupleType table_customer::Read(
    size_t tid
) {
    TupleType result;
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        std::get<0>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        std::get<1>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(2).second);
        std::get<2>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<16>>&>(getColumn(3).second);
        std::get<3>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Char<2>>&>(getColumn(4).second);
        std::get<4>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<16>>&>(getColumn(5).second);
        std::get<5>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(6).second);
        std::get<6>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(7).second);
        std::get<7>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(8).second);
        std::get<8>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Char<2>>&>(getColumn(9).second);
        std::get<9>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Char<9>>&>(getColumn(10).second);
        std::get<10>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Char<16>>&>(getColumn(11).second);
        std::get<11>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Timestamp>&>(getColumn(12).second);
        std::get<12>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Char<2>>&>(getColumn(13).second);
        std::get<13>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<12,2>>&>(getColumn(14).second);
        std::get<14>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,4>>&>(getColumn(15).second);
        std::get<15>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<12,2>>&>(getColumn(16).second);
        std::get<16>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<12,2>>&>(getColumn(17).second);
        std::get<17>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,0>>&>(getColumn(18).second);
        std::get<18>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,0>>&>(getColumn(19).second);
        std::get<19>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<500>>&>(getColumn(20).second);
        std::get<20>(result) = col.getStorage()[tid];
    }
    return result;
}

void table_customer::Update(
size_t tid,
    const TupleType& newRow
) {
    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map.erase(pk);
    auto pk_new = Assemble(newRow);
    pk_map[pk_new] = tid;
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        col.getStorage()[tid] = (std::get<0>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        col.getStorage()[tid] = (std::get<1>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(2).second);
        col.getStorage()[tid] = (std::get<2>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<16>>&>(getColumn(3).second);
        col.getStorage()[tid] = (std::get<3>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<2>>&>(getColumn(4).second);
        col.getStorage()[tid] = (std::get<4>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<16>>&>(getColumn(5).second);
        col.getStorage()[tid] = (std::get<5>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(6).second);
        col.getStorage()[tid] = (std::get<6>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(7).second);
        col.getStorage()[tid] = (std::get<7>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<20>>&>(getColumn(8).second);
        col.getStorage()[tid] = (std::get<8>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<2>>&>(getColumn(9).second);
        col.getStorage()[tid] = (std::get<9>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<9>>&>(getColumn(10).second);
        col.getStorage()[tid] = (std::get<10>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<16>>&>(getColumn(11).second);
        col.getStorage()[tid] = (std::get<11>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Timestamp>&>(getColumn(12).second);
        col.getStorage()[tid] = (std::get<12>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<2>>&>(getColumn(13).second);
        col.getStorage()[tid] = (std::get<13>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<12,2>>&>(getColumn(14).second);
        col.getStorage()[tid] = (std::get<14>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,4>>&>(getColumn(15).second);
        col.getStorage()[tid] = (std::get<15>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<12,2>>&>(getColumn(16).second);
        col.getStorage()[tid] = (std::get<16>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<12,2>>&>(getColumn(17).second);
        col.getStorage()[tid] = (std::get<17>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,0>>&>(getColumn(18).second);
        col.getStorage()[tid] = (std::get<18>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,0>>&>(getColumn(19).second);
        col.getStorage()[tid] = (std::get<19>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<500>>&>(getColumn(20).second);
        col.getStorage()[tid] = (std::get<20>(newRow));
    }
}

void table_customer::Delete(
    size_t tid
) {
    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map.erase(pk);
    dropRow(tid);
}

void table_customer::addLastRowToIndex(
) {
    auto tid = getSize() - 1;    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map[pk] = tid;
}

size_t table_customer::Lookup(
    const PKType& key
) {
    return pk_map[key];
}

table_customer::PKType table_customer::Assemble(
    TupleType row
) {
    PKType result;
    std::get<0>(result) = std::get<2>(row);
    std::get<1>(result) = std::get<1>(row);
    std::get<2>(result) = std::get<0>(row);
    return result;
}

std::optional<table_customer::TupleType> table_customer::KeyIterator::next() {
    if (it_start != it_end) {
        TupleType result;
        size_t tid = it_start->second;
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(0).second);
            std::get<0>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(1).second);
            std::get<1>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(2).second);
            std::get<2>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Varchar<16>>&>(table.getColumn(3).second);
            std::get<3>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Char<2>>&>(table.getColumn(4).second);
            std::get<4>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Varchar<16>>&>(table.getColumn(5).second);
            std::get<5>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Varchar<20>>&>(table.getColumn(6).second);
            std::get<6>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Varchar<20>>&>(table.getColumn(7).second);
            std::get<7>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Varchar<20>>&>(table.getColumn(8).second);
            std::get<8>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Char<2>>&>(table.getColumn(9).second);
            std::get<9>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Char<9>>&>(table.getColumn(10).second);
            std::get<10>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Char<16>>&>(table.getColumn(11).second);
            std::get<11>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Timestamp>&>(table.getColumn(12).second);
            std::get<12>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Char<2>>&>(table.getColumn(13).second);
            std::get<13>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Numeric<12,2>>&>(table.getColumn(14).second);
            std::get<14>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Numeric<4,4>>&>(table.getColumn(15).second);
            std::get<15>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Numeric<12,2>>&>(table.getColumn(16).second);
            std::get<16>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Numeric<12,2>>&>(table.getColumn(17).second);
            std::get<17>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Numeric<4,0>>&>(table.getColumn(18).second);
            std::get<18>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Numeric<4,0>>&>(table.getColumn(19).second);
            std::get<19>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Varchar<500>>&>(table.getColumn(20).second);
            std::get<20>(result) = col.getStorage()[tid];
        }
        it_start++;
        return result;
    }
    return std::nullopt;
}

void table_history::Create(
    const TupleType& newRow
) {
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        col.getStorage().push_back(std::get<0>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        col.getStorage().push_back(std::get<1>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(2).second);
        col.getStorage().push_back(std::get<2>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(3).second);
        col.getStorage().push_back(std::get<3>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(4).second);
        col.getStorage().push_back(std::get<4>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Timestamp>&>(getColumn(5).second);
        col.getStorage().push_back(std::get<5>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<6,2>>&>(getColumn(6).second);
        col.getStorage().push_back(std::get<6>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<24>>&>(getColumn(7).second);
        col.getStorage().push_back(std::get<7>(newRow));
    }
    appendRow();
}

table_history::TupleType table_history::Read(
    size_t tid
) {
    TupleType result;
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        std::get<0>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        std::get<1>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(2).second);
        std::get<2>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(3).second);
        std::get<3>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(4).second);
        std::get<4>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Timestamp>&>(getColumn(5).second);
        std::get<5>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<6,2>>&>(getColumn(6).second);
        std::get<6>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<24>>&>(getColumn(7).second);
        std::get<7>(result) = col.getStorage()[tid];
    }
    return result;
}

void table_history::Update(
size_t tid,
    const TupleType& newRow
) {
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        col.getStorage()[tid] = (std::get<0>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        col.getStorage()[tid] = (std::get<1>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(2).second);
        col.getStorage()[tid] = (std::get<2>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(3).second);
        col.getStorage()[tid] = (std::get<3>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(4).second);
        col.getStorage()[tid] = (std::get<4>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Timestamp>&>(getColumn(5).second);
        col.getStorage()[tid] = (std::get<5>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<6,2>>&>(getColumn(6).second);
        col.getStorage()[tid] = (std::get<6>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<24>>&>(getColumn(7).second);
        col.getStorage()[tid] = (std::get<7>(newRow));
    }
}

void table_history::Delete(
    size_t tid
) {
    dropRow(tid);
}

void table_history::addLastRowToIndex(
) {
}

void table_neworder::Create(
    const TupleType& newRow
) {
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        col.getStorage().push_back(std::get<0>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        col.getStorage().push_back(std::get<1>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(2).second);
        col.getStorage().push_back(std::get<2>(newRow));
    }
    auto pk = Assemble(newRow);
    auto tid = getSize() - 1;
    pk_map[pk] = tid;
    appendRow();
}

table_neworder::TupleType table_neworder::Read(
    size_t tid
) {
    TupleType result;
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        std::get<0>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        std::get<1>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(2).second);
        std::get<2>(result) = col.getStorage()[tid];
    }
    return result;
}

void table_neworder::Update(
size_t tid,
    const TupleType& newRow
) {
    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map.erase(pk);
    auto pk_new = Assemble(newRow);
    pk_map[pk_new] = tid;
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        col.getStorage()[tid] = (std::get<0>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        col.getStorage()[tid] = (std::get<1>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(2).second);
        col.getStorage()[tid] = (std::get<2>(newRow));
    }
}

void table_neworder::Delete(
    size_t tid
) {
    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map.erase(pk);
    dropRow(tid);
}

void table_neworder::addLastRowToIndex(
) {
    auto tid = getSize() - 1;    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map[pk] = tid;
}

size_t table_neworder::Lookup(
    const PKType& key
) {
    return pk_map[key];
}

table_neworder::PKType table_neworder::Assemble(
    TupleType row
) {
    PKType result;
    std::get<0>(result) = std::get<2>(row);
    std::get<1>(result) = std::get<1>(row);
    std::get<2>(result) = std::get<0>(row);
    return result;
}

std::optional<table_neworder::TupleType> table_neworder::KeyIterator::next() {
    if (it_start != it_end) {
        TupleType result;
        size_t tid = it_start->second;
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(0).second);
            std::get<0>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(1).second);
            std::get<1>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(2).second);
            std::get<2>(result) = col.getStorage()[tid];
        }
        it_start++;
        return result;
    }
    return std::nullopt;
}

void table_order::Create(
    const TupleType& newRow
) {
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        col.getStorage().push_back(std::get<0>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        col.getStorage().push_back(std::get<1>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(2).second);
        col.getStorage().push_back(std::get<2>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(3).second);
        col.getStorage().push_back(std::get<3>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Timestamp>&>(getColumn(4).second);
        col.getStorage().push_back(std::get<4>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(5).second);
        col.getStorage().push_back(std::get<5>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<2,0>>&>(getColumn(6).second);
        col.getStorage().push_back(std::get<6>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<1,0>>&>(getColumn(7).second);
        col.getStorage().push_back(std::get<7>(newRow));
    }
    auto pk = Assemble(newRow);
    auto tid = getSize() - 1;
    pk_map[pk] = tid;
    appendRow();
}

table_order::TupleType table_order::Read(
    size_t tid
) {
    TupleType result;
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        std::get<0>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        std::get<1>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(2).second);
        std::get<2>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(3).second);
        std::get<3>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Timestamp>&>(getColumn(4).second);
        std::get<4>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(5).second);
        std::get<5>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<2,0>>&>(getColumn(6).second);
        std::get<6>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<1,0>>&>(getColumn(7).second);
        std::get<7>(result) = col.getStorage()[tid];
    }
    return result;
}

void table_order::Update(
size_t tid,
    const TupleType& newRow
) {
    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map.erase(pk);
    auto pk_new = Assemble(newRow);
    pk_map[pk_new] = tid;
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        col.getStorage()[tid] = (std::get<0>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        col.getStorage()[tid] = (std::get<1>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(2).second);
        col.getStorage()[tid] = (std::get<2>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(3).second);
        col.getStorage()[tid] = (std::get<3>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Timestamp>&>(getColumn(4).second);
        col.getStorage()[tid] = (std::get<4>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(5).second);
        col.getStorage()[tid] = (std::get<5>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<2,0>>&>(getColumn(6).second);
        col.getStorage()[tid] = (std::get<6>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<1,0>>&>(getColumn(7).second);
        col.getStorage()[tid] = (std::get<7>(newRow));
    }
}

void table_order::Delete(
    size_t tid
) {
    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map.erase(pk);
    dropRow(tid);
}

void table_order::addLastRowToIndex(
) {
    auto tid = getSize() - 1;    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map[pk] = tid;
}

size_t table_order::Lookup(
    const PKType& key
) {
    return pk_map[key];
}

table_order::PKType table_order::Assemble(
    TupleType row
) {
    PKType result;
    std::get<0>(result) = std::get<2>(row);
    std::get<1>(result) = std::get<1>(row);
    std::get<2>(result) = std::get<0>(row);
    return result;
}

std::optional<table_order::TupleType> table_order::KeyIterator::next() {
    if (it_start != it_end) {
        TupleType result;
        size_t tid = it_start->second;
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(0).second);
            std::get<0>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(1).second);
            std::get<1>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(2).second);
            std::get<2>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(3).second);
            std::get<3>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Timestamp>&>(table.getColumn(4).second);
            std::get<4>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(5).second);
            std::get<5>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Numeric<2,0>>&>(table.getColumn(6).second);
            std::get<6>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Numeric<1,0>>&>(table.getColumn(7).second);
            std::get<7>(result) = col.getStorage()[tid];
        }
        it_start++;
        return result;
    }
    return std::nullopt;
}

void table_orderline::Create(
    const TupleType& newRow
) {
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        col.getStorage().push_back(std::get<0>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        col.getStorage().push_back(std::get<1>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(2).second);
        col.getStorage().push_back(std::get<2>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(3).second);
        col.getStorage().push_back(std::get<3>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(4).second);
        col.getStorage().push_back(std::get<4>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(5).second);
        col.getStorage().push_back(std::get<5>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Timestamp>&>(getColumn(6).second);
        col.getStorage().push_back(std::get<6>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<2,0>>&>(getColumn(7).second);
        col.getStorage().push_back(std::get<7>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<6,2>>&>(getColumn(8).second);
        col.getStorage().push_back(std::get<8>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(9).second);
        col.getStorage().push_back(std::get<9>(newRow));
    }
    auto pk = Assemble(newRow);
    auto tid = getSize() - 1;
    pk_map[pk] = tid;
    appendRow();
}

table_orderline::TupleType table_orderline::Read(
    size_t tid
) {
    TupleType result;
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        std::get<0>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        std::get<1>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(2).second);
        std::get<2>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(3).second);
        std::get<3>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(4).second);
        std::get<4>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(5).second);
        std::get<5>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Timestamp>&>(getColumn(6).second);
        std::get<6>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<2,0>>&>(getColumn(7).second);
        std::get<7>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<6,2>>&>(getColumn(8).second);
        std::get<8>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(9).second);
        std::get<9>(result) = col.getStorage()[tid];
    }
    return result;
}

void table_orderline::Update(
size_t tid,
    const TupleType& newRow
) {
    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map.erase(pk);
    auto pk_new = Assemble(newRow);
    pk_map[pk_new] = tid;
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        col.getStorage()[tid] = (std::get<0>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        col.getStorage()[tid] = (std::get<1>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(2).second);
        col.getStorage()[tid] = (std::get<2>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(3).second);
        col.getStorage()[tid] = (std::get<3>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(4).second);
        col.getStorage()[tid] = (std::get<4>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(5).second);
        col.getStorage()[tid] = (std::get<5>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Timestamp>&>(getColumn(6).second);
        col.getStorage()[tid] = (std::get<6>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<2,0>>&>(getColumn(7).second);
        col.getStorage()[tid] = (std::get<7>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<6,2>>&>(getColumn(8).second);
        col.getStorage()[tid] = (std::get<8>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(9).second);
        col.getStorage()[tid] = (std::get<9>(newRow));
    }
}

void table_orderline::Delete(
    size_t tid
) {
    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map.erase(pk);
    dropRow(tid);
}

void table_orderline::addLastRowToIndex(
) {
    auto tid = getSize() - 1;    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map[pk] = tid;
}

size_t table_orderline::Lookup(
    const PKType& key
) {
    return pk_map[key];
}

table_orderline::PKType table_orderline::Assemble(
    TupleType row
) {
    PKType result;
    std::get<0>(result) = std::get<2>(row);
    std::get<1>(result) = std::get<1>(row);
    std::get<2>(result) = std::get<0>(row);
    std::get<3>(result) = std::get<3>(row);
    return result;
}

std::optional<table_orderline::TupleType> table_orderline::KeyIterator::next() {
    if (it_start != it_end) {
        TupleType result;
        size_t tid = it_start->second;
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(0).second);
            std::get<0>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(1).second);
            std::get<1>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(2).second);
            std::get<2>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(3).second);
            std::get<3>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(4).second);
            std::get<4>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(5).second);
            std::get<5>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Timestamp>&>(table.getColumn(6).second);
            std::get<6>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Numeric<2,0>>&>(table.getColumn(7).second);
            std::get<7>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Numeric<6,2>>&>(table.getColumn(8).second);
            std::get<8>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Char<24>>&>(table.getColumn(9).second);
            std::get<9>(result) = col.getStorage()[tid];
        }
        it_start++;
        return result;
    }
    return std::nullopt;
}

void table_item::Create(
    const TupleType& newRow
) {
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        col.getStorage().push_back(std::get<0>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        col.getStorage().push_back(std::get<1>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<24>>&>(getColumn(2).second);
        col.getStorage().push_back(std::get<2>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<5,2>>&>(getColumn(3).second);
        col.getStorage().push_back(std::get<3>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<50>>&>(getColumn(4).second);
        col.getStorage().push_back(std::get<4>(newRow));
    }
    auto pk = Assemble(newRow);
    auto tid = getSize() - 1;
    pk_map[pk] = tid;
    appendRow();
}

table_item::TupleType table_item::Read(
    size_t tid
) {
    TupleType result;
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        std::get<0>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        std::get<1>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<24>>&>(getColumn(2).second);
        std::get<2>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<5,2>>&>(getColumn(3).second);
        std::get<3>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<50>>&>(getColumn(4).second);
        std::get<4>(result) = col.getStorage()[tid];
    }
    return result;
}

void table_item::Update(
size_t tid,
    const TupleType& newRow
) {
    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map.erase(pk);
    auto pk_new = Assemble(newRow);
    pk_map[pk_new] = tid;
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        col.getStorage()[tid] = (std::get<0>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        col.getStorage()[tid] = (std::get<1>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<24>>&>(getColumn(2).second);
        col.getStorage()[tid] = (std::get<2>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<5,2>>&>(getColumn(3).second);
        col.getStorage()[tid] = (std::get<3>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<50>>&>(getColumn(4).second);
        col.getStorage()[tid] = (std::get<4>(newRow));
    }
}

void table_item::Delete(
    size_t tid
) {
    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map.erase(pk);
    dropRow(tid);
}

void table_item::addLastRowToIndex(
) {
    auto tid = getSize() - 1;    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map[pk] = tid;
}

size_t table_item::Lookup(
    const PKType& key
) {
    return pk_map[key];
}

table_item::PKType table_item::Assemble(
    TupleType row
) {
    PKType result;
    std::get<0>(result) = std::get<0>(row);
    return result;
}

std::optional<table_item::TupleType> table_item::KeyIterator::next() {
    if (it_start != it_end) {
        TupleType result;
        size_t tid = it_start->second;
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(0).second);
            std::get<0>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(1).second);
            std::get<1>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Varchar<24>>&>(table.getColumn(2).second);
            std::get<2>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Numeric<5,2>>&>(table.getColumn(3).second);
            std::get<3>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Varchar<50>>&>(table.getColumn(4).second);
            std::get<4>(result) = col.getStorage()[tid];
        }
        it_start++;
        return result;
    }
    return std::nullopt;
}

void table_stock::Create(
    const TupleType& newRow
) {
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        col.getStorage().push_back(std::get<0>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        col.getStorage().push_back(std::get<1>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,0>>&>(getColumn(2).second);
        col.getStorage().push_back(std::get<2>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(3).second);
        col.getStorage().push_back(std::get<3>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(4).second);
        col.getStorage().push_back(std::get<4>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(5).second);
        col.getStorage().push_back(std::get<5>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(6).second);
        col.getStorage().push_back(std::get<6>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(7).second);
        col.getStorage().push_back(std::get<7>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(8).second);
        col.getStorage().push_back(std::get<8>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(9).second);
        col.getStorage().push_back(std::get<9>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(10).second);
        col.getStorage().push_back(std::get<10>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(11).second);
        col.getStorage().push_back(std::get<11>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(12).second);
        col.getStorage().push_back(std::get<12>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<8,0>>&>(getColumn(13).second);
        col.getStorage().push_back(std::get<13>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,0>>&>(getColumn(14).second);
        col.getStorage().push_back(std::get<14>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,0>>&>(getColumn(15).second);
        col.getStorage().push_back(std::get<15>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<50>>&>(getColumn(16).second);
        col.getStorage().push_back(std::get<16>(newRow));
    }
    auto pk = Assemble(newRow);
    auto tid = getSize() - 1;
    pk_map[pk] = tid;
    appendRow();
}

table_stock::TupleType table_stock::Read(
    size_t tid
) {
    TupleType result;
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        std::get<0>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        std::get<1>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,0>>&>(getColumn(2).second);
        std::get<2>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(3).second);
        std::get<3>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(4).second);
        std::get<4>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(5).second);
        std::get<5>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(6).second);
        std::get<6>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(7).second);
        std::get<7>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(8).second);
        std::get<8>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(9).second);
        std::get<9>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(10).second);
        std::get<10>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(11).second);
        std::get<11>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(12).second);
        std::get<12>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<8,0>>&>(getColumn(13).second);
        std::get<13>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,0>>&>(getColumn(14).second);
        std::get<14>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,0>>&>(getColumn(15).second);
        std::get<15>(result) = col.getStorage()[tid];
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<50>>&>(getColumn(16).second);
        std::get<16>(result) = col.getStorage()[tid];
    }
    return result;
}

void table_stock::Update(
size_t tid,
    const TupleType& newRow
) {
    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map.erase(pk);
    auto pk_new = Assemble(newRow);
    pk_map[pk_new] = tid;
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(0).second);
        col.getStorage()[tid] = (std::get<0>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Integer>&>(getColumn(1).second);
        col.getStorage()[tid] = (std::get<1>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,0>>&>(getColumn(2).second);
        col.getStorage()[tid] = (std::get<2>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(3).second);
        col.getStorage()[tid] = (std::get<3>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(4).second);
        col.getStorage()[tid] = (std::get<4>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(5).second);
        col.getStorage()[tid] = (std::get<5>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(6).second);
        col.getStorage()[tid] = (std::get<6>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(7).second);
        col.getStorage()[tid] = (std::get<7>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(8).second);
        col.getStorage()[tid] = (std::get<8>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(9).second);
        col.getStorage()[tid] = (std::get<9>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(10).second);
        col.getStorage()[tid] = (std::get<10>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(11).second);
        col.getStorage()[tid] = (std::get<11>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Char<24>>&>(getColumn(12).second);
        col.getStorage()[tid] = (std::get<12>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<8,0>>&>(getColumn(13).second);
        col.getStorage()[tid] = (std::get<13>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,0>>&>(getColumn(14).second);
        col.getStorage()[tid] = (std::get<14>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Numeric<4,0>>&>(getColumn(15).second);
        col.getStorage()[tid] = (std::get<15>(newRow));
    }
    {
        auto& col = static_cast<TypedColumn<Varchar<50>>&>(getColumn(16).second);
        col.getStorage()[tid] = (std::get<16>(newRow));
    }
}

void table_stock::Delete(
    size_t tid
) {
    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map.erase(pk);
    dropRow(tid);
}

void table_stock::addLastRowToIndex(
) {
    auto tid = getSize() - 1;    auto row = Read(tid);
    auto pk = Assemble(row);
    pk_map[pk] = tid;
}

size_t table_stock::Lookup(
    const PKType& key
) {
    return pk_map[key];
}

table_stock::PKType table_stock::Assemble(
    TupleType row
) {
    PKType result;
    std::get<0>(result) = std::get<1>(row);
    std::get<1>(result) = std::get<0>(row);
    return result;
}

std::optional<table_stock::TupleType> table_stock::KeyIterator::next() {
    if (it_start != it_end) {
        TupleType result;
        size_t tid = it_start->second;
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(0).second);
            std::get<0>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Integer>&>(table.getColumn(1).second);
            std::get<1>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Numeric<4,0>>&>(table.getColumn(2).second);
            std::get<2>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Char<24>>&>(table.getColumn(3).second);
            std::get<3>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Char<24>>&>(table.getColumn(4).second);
            std::get<4>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Char<24>>&>(table.getColumn(5).second);
            std::get<5>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Char<24>>&>(table.getColumn(6).second);
            std::get<6>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Char<24>>&>(table.getColumn(7).second);
            std::get<7>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Char<24>>&>(table.getColumn(8).second);
            std::get<8>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Char<24>>&>(table.getColumn(9).second);
            std::get<9>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Char<24>>&>(table.getColumn(10).second);
            std::get<10>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Char<24>>&>(table.getColumn(11).second);
            std::get<11>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Char<24>>&>(table.getColumn(12).second);
            std::get<12>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Numeric<8,0>>&>(table.getColumn(13).second);
            std::get<13>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Numeric<4,0>>&>(table.getColumn(14).second);
            std::get<14>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Numeric<4,0>>&>(table.getColumn(15).second);
            std::get<15>(result) = col.getStorage()[tid];
        }
        {
            auto& col = static_cast<TypedColumn<Varchar<50>>&>(table.getColumn(16).second);
            std::get<16>(result) = col.getStorage()[tid];
        }
        it_start++;
        return result;
    }
    return std::nullopt;
}

} // namespace imlab
