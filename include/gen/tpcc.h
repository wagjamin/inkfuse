
#pragma once

#include "imlab/storage/relation.h"
#include "imlab/infra/types.h"
#include <tuple>
#include <map>
#include <cstddef>
#include <iterator>

namespace imlab {

struct table_warehouse final : public StoredRelation {
public:
~table_warehouse() override = default;
using TupleType = std::tuple<
    Integer,
    Varchar<10>,
    Varchar<20>,
    Varchar<20>,
    Varchar<20>,
    Char<2>,
    Char<9>,
    Numeric<4,4>,
    Numeric<12,2>
>;
using PKType = std::tuple<
    Integer
>;
table_warehouse();
void Create(
    const TupleType& newRow
);
TupleType Read(
    size_t tid
);
void Update(
    size_t tid,
    const TupleType& newRow
);
void Delete(
    size_t tid
);
virtual void addLastRowToIndex(
) override;
size_t Lookup(
    const PKType& key
);


// Custom, non standard compatible iterator.
struct KeyIterator
{
public:
    // Create [start, end[ iterator into the the relation.
KeyIterator(table_warehouse& table_, PKType start_, PKType end_): table(table_), it_start(table.pk_map.lower_bound(start_)), it_end(table.pk_map.upper_bound(end_)) {
}

std::optional<TupleType> next();
private:
table_warehouse& table;
std::map<PKType, size_t>::iterator it_start;
std::map<PKType, size_t>::iterator it_end;
};

static PKType Assemble(TupleType row);

private:
std::map<PKType, size_t> pk_map;

};

struct table_district final : public StoredRelation {
public:
~table_district() override = default;
using TupleType = std::tuple<
    Integer,
    Integer,
    Varchar<10>,
    Varchar<20>,
    Varchar<20>,
    Varchar<20>,
    Char<2>,
    Char<9>,
    Numeric<4,4>,
    Numeric<12,2>,
    Integer
>;
using PKType = std::tuple<
    Integer,
    Integer
>;
table_district();
void Create(
    const TupleType& newRow
);
TupleType Read(
    size_t tid
);
void Update(
    size_t tid,
    const TupleType& newRow
);
void Delete(
    size_t tid
);
virtual void addLastRowToIndex(
) override;
size_t Lookup(
    const PKType& key
);


// Custom, non standard compatible iterator.
struct KeyIterator
{
public:
    // Create [start, end[ iterator into the the relation.
KeyIterator(table_district& table_, PKType start_, PKType end_): table(table_), it_start(table.pk_map.lower_bound(start_)), it_end(table.pk_map.upper_bound(end_)) {
}

std::optional<TupleType> next();
private:
table_district& table;
std::map<PKType, size_t>::iterator it_start;
std::map<PKType, size_t>::iterator it_end;
};

static PKType Assemble(TupleType row);

private:
std::map<PKType, size_t> pk_map;

};

struct table_customer final : public StoredRelation {
public:
~table_customer() override = default;
using TupleType = std::tuple<
    Integer,
    Integer,
    Integer,
    Varchar<16>,
    Char<2>,
    Varchar<16>,
    Varchar<20>,
    Varchar<20>,
    Varchar<20>,
    Char<2>,
    Char<9>,
    Char<16>,
    Timestamp,
    Char<2>,
    Numeric<12,2>,
    Numeric<4,4>,
    Numeric<12,2>,
    Numeric<12,2>,
    Numeric<4,0>,
    Numeric<4,0>,
    Varchar<500>
>;
using PKType = std::tuple<
    Integer,
    Integer,
    Integer
>;
table_customer();
void Create(
    const TupleType& newRow
);
TupleType Read(
    size_t tid
);
void Update(
    size_t tid,
    const TupleType& newRow
);
void Delete(
    size_t tid
);
virtual void addLastRowToIndex(
) override;
size_t Lookup(
    const PKType& key
);


// Custom, non standard compatible iterator.
struct KeyIterator
{
public:
    // Create [start, end[ iterator into the the relation.
KeyIterator(table_customer& table_, PKType start_, PKType end_): table(table_), it_start(table.pk_map.lower_bound(start_)), it_end(table.pk_map.upper_bound(end_)) {
}

std::optional<TupleType> next();
private:
table_customer& table;
std::map<PKType, size_t>::iterator it_start;
std::map<PKType, size_t>::iterator it_end;
};

static PKType Assemble(TupleType row);

private:
std::map<PKType, size_t> pk_map;

};

struct table_history final : public StoredRelation {
public:
~table_history() override = default;
using TupleType = std::tuple<
    Integer,
    Integer,
    Integer,
    Integer,
    Integer,
    Timestamp,
    Numeric<6,2>,
    Varchar<24>
>;
table_history();
void Create(
    const TupleType& newRow
);
TupleType Read(
    size_t tid
);
void Update(
    size_t tid,
    const TupleType& newRow
);
void Delete(
    size_t tid
);
virtual void addLastRowToIndex(
) override;
private:
};

struct table_neworder final : public StoredRelation {
public:
~table_neworder() override = default;
using TupleType = std::tuple<
    Integer,
    Integer,
    Integer
>;
using PKType = std::tuple<
    Integer,
    Integer,
    Integer
>;
table_neworder();
void Create(
    const TupleType& newRow
);
TupleType Read(
    size_t tid
);
void Update(
    size_t tid,
    const TupleType& newRow
);
void Delete(
    size_t tid
);
virtual void addLastRowToIndex(
) override;
size_t Lookup(
    const PKType& key
);


// Custom, non standard compatible iterator.
struct KeyIterator
{
public:
    // Create [start, end[ iterator into the the relation.
KeyIterator(table_neworder& table_, PKType start_, PKType end_): table(table_), it_start(table.pk_map.lower_bound(start_)), it_end(table.pk_map.upper_bound(end_)) {
}

std::optional<TupleType> next();
private:
table_neworder& table;
std::map<PKType, size_t>::iterator it_start;
std::map<PKType, size_t>::iterator it_end;
};

static PKType Assemble(TupleType row);

private:
std::map<PKType, size_t> pk_map;

};

struct table_order final : public StoredRelation {
public:
~table_order() override = default;
using TupleType = std::tuple<
    Integer,
    Integer,
    Integer,
    Integer,
    Timestamp,
    Integer,
    Numeric<2,0>,
    Numeric<1,0>
>;
using PKType = std::tuple<
    Integer,
    Integer,
    Integer
>;
table_order();
void Create(
    const TupleType& newRow
);
TupleType Read(
    size_t tid
);
void Update(
    size_t tid,
    const TupleType& newRow
);
void Delete(
    size_t tid
);
virtual void addLastRowToIndex(
) override;
size_t Lookup(
    const PKType& key
);


// Custom, non standard compatible iterator.
struct KeyIterator
{
public:
    // Create [start, end[ iterator into the the relation.
KeyIterator(table_order& table_, PKType start_, PKType end_): table(table_), it_start(table.pk_map.lower_bound(start_)), it_end(table.pk_map.upper_bound(end_)) {
}

std::optional<TupleType> next();
private:
table_order& table;
std::map<PKType, size_t>::iterator it_start;
std::map<PKType, size_t>::iterator it_end;
};

static PKType Assemble(TupleType row);

private:
std::map<PKType, size_t> pk_map;

};

struct table_orderline final : public StoredRelation {
public:
~table_orderline() override = default;
using TupleType = std::tuple<
    Integer,
    Integer,
    Integer,
    Integer,
    Integer,
    Integer,
    Timestamp,
    Numeric<2,0>,
    Numeric<6,2>,
    Char<24>
>;
using PKType = std::tuple<
    Integer,
    Integer,
    Integer,
    Integer
>;
table_orderline();
void Create(
    const TupleType& newRow
);
TupleType Read(
    size_t tid
);
void Update(
    size_t tid,
    const TupleType& newRow
);
void Delete(
    size_t tid
);
virtual void addLastRowToIndex(
) override;
size_t Lookup(
    const PKType& key
);


// Custom, non standard compatible iterator.
struct KeyIterator
{
public:
    // Create [start, end[ iterator into the the relation.
KeyIterator(table_orderline& table_, PKType start_, PKType end_): table(table_), it_start(table.pk_map.lower_bound(start_)), it_end(table.pk_map.upper_bound(end_)) {
}

std::optional<TupleType> next();
private:
table_orderline& table;
std::map<PKType, size_t>::iterator it_start;
std::map<PKType, size_t>::iterator it_end;
};

static PKType Assemble(TupleType row);

private:
std::map<PKType, size_t> pk_map;

};

struct table_item final : public StoredRelation {
public:
~table_item() override = default;
using TupleType = std::tuple<
    Integer,
    Integer,
    Varchar<24>,
    Numeric<5,2>,
    Varchar<50>
>;
using PKType = std::tuple<
    Integer
>;
table_item();
void Create(
    const TupleType& newRow
);
TupleType Read(
    size_t tid
);
void Update(
    size_t tid,
    const TupleType& newRow
);
void Delete(
    size_t tid
);
virtual void addLastRowToIndex(
) override;
size_t Lookup(
    const PKType& key
);


// Custom, non standard compatible iterator.
struct KeyIterator
{
public:
    // Create [start, end[ iterator into the the relation.
KeyIterator(table_item& table_, PKType start_, PKType end_): table(table_), it_start(table.pk_map.lower_bound(start_)), it_end(table.pk_map.upper_bound(end_)) {
}

std::optional<TupleType> next();
private:
table_item& table;
std::map<PKType, size_t>::iterator it_start;
std::map<PKType, size_t>::iterator it_end;
};

static PKType Assemble(TupleType row);

private:
std::map<PKType, size_t> pk_map;

};

struct table_stock final : public StoredRelation {
public:
~table_stock() override = default;
using TupleType = std::tuple<
    Integer,
    Integer,
    Numeric<4,0>,
    Char<24>,
    Char<24>,
    Char<24>,
    Char<24>,
    Char<24>,
    Char<24>,
    Char<24>,
    Char<24>,
    Char<24>,
    Char<24>,
    Numeric<8,0>,
    Numeric<4,0>,
    Numeric<4,0>,
    Varchar<50>
>;
using PKType = std::tuple<
    Integer,
    Integer
>;
table_stock();
void Create(
    const TupleType& newRow
);
TupleType Read(
    size_t tid
);
void Update(
    size_t tid,
    const TupleType& newRow
);
void Delete(
    size_t tid
);
virtual void addLastRowToIndex(
) override;
size_t Lookup(
    const PKType& key
);


// Custom, non standard compatible iterator.
struct KeyIterator
{
public:
    // Create [start, end[ iterator into the the relation.
KeyIterator(table_stock& table_, PKType start_, PKType end_): table(table_), it_start(table.pk_map.lower_bound(start_)), it_end(table.pk_map.upper_bound(end_)) {
}

std::optional<TupleType> next();
private:
table_stock& table;
std::map<PKType, size_t>::iterator it_start;
std::map<PKType, size_t>::iterator it_end;
};

static PKType Assemble(TupleType row);

private:
std::map<PKType, size_t> pk_map;

};



} // namespace imlab

