// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------

#include <sstream>
#include "imlab/schemac/schema_parse_context.h"
#include "gtest/gtest.h"

using Type = imlab::schemac::Type;
using IndexType = imlab::schemac::IndexType;
using SchemaParseContext = imlab::schemac::SchemaParseContext;

namespace {

TEST(SchemaParseContextTest, ParseEmptyString) {
    std::istringstream in("");
    SchemaParseContext spc;
    auto schema = spc.Parse(in);
}

TEST(SchemaParseContextTest, ParseEmptyTable) {
    std::istringstream in("create table foo ();");
    SchemaParseContext spc;
    auto schema = spc.Parse(in);
    auto &tables = schema.tables;
    ASSERT_EQ(tables.size(), 1);
    EXPECT_EQ(tables[0].id, "foo");
    EXPECT_TRUE(tables[0].columns.empty());
    EXPECT_TRUE(tables[0].primary_key.empty());
}

TEST(SchemaParseContextTest, ParseEmptyTableWithIndexType) {
    std::istringstream in("create table foo () with (key_index_type = unordered_map);");
    SchemaParseContext spc;
    auto schema = spc.Parse(in);
    auto &tables = schema.tables;
    ASSERT_EQ(tables.size(), 1);
    EXPECT_EQ(tables[0].id, "foo");
    EXPECT_EQ(tables[0].index_type, IndexType::kSTLUnorderedMap);
    EXPECT_TRUE(tables[0].columns.empty());
    EXPECT_TRUE(tables[0].primary_key.empty());
}

TEST(SchemaParseContextTest, ParseTwoEmptyTables) {
    std::istringstream in("create table foo (); create table bar ();");
    SchemaParseContext spc;
    auto schema = spc.Parse(in);
    auto &tables = schema.tables;
    ASSERT_EQ(tables.size(), 2);
    EXPECT_EQ(tables[0].id, "foo");
    EXPECT_TRUE(tables[0].columns.empty());
    EXPECT_TRUE(tables[0].primary_key.empty());
    EXPECT_EQ(tables[1].id, "bar");
    EXPECT_TRUE(tables[1].columns.empty());
    EXPECT_TRUE(tables[1].primary_key.empty());
}

TEST(SchemaParseContextTest, ParseSingleIntegerColumn) {
    std::istringstream in(R"SQLSCHEMA(
        create table foo ( bar integer not null );
    )SQLSCHEMA");
    SchemaParseContext spc;
    auto schema = spc.Parse(in);
    auto &tables = schema.tables;
    ASSERT_EQ(tables.size(), 1);
    EXPECT_EQ(tables[0].id, "foo");
    ASSERT_EQ(tables[0].columns.size(), 1);
    EXPECT_EQ(tables[0].columns[0].id, "bar");
    EXPECT_EQ(tables[0].columns[0].type.tclass, Type::kInteger);
    EXPECT_TRUE(tables[0].primary_key.empty());
}

TEST(SchemaParseContextTest, ParseSingleTimestampColumn) {
    std::istringstream in(R"SQLSCHEMA(
        create table foo ( bar timestamp not null );
    )SQLSCHEMA");
    SchemaParseContext spc;
    auto schema = spc.Parse(in);
    auto &tables = schema.tables;
    ASSERT_EQ(tables.size(), 1);
    EXPECT_EQ(tables[0].id, "foo");
    ASSERT_EQ(tables[0].columns.size(), 1);
    EXPECT_EQ(tables[0].columns[0].id, "bar");
    EXPECT_EQ(tables[0].columns[0].type.tclass, Type::kTimestamp);
    EXPECT_TRUE(tables[0].primary_key.empty());
}

TEST(SchemaParseContextTest, ParseSingleNumericColumn) {
    std::istringstream in(R"SQLSCHEMA(
        create table foo ( bar numeric(4, 2) not null );
    )SQLSCHEMA");
    SchemaParseContext spc;
    auto schema = spc.Parse(in);
    auto &tables = schema.tables;
    ASSERT_EQ(tables.size(), 1);
    EXPECT_EQ(tables[0].id, "foo");
    ASSERT_EQ(tables[0].columns.size(), 1);
    EXPECT_EQ(tables[0].columns[0].id, "bar");
    EXPECT_EQ(tables[0].columns[0].type.tclass, Type::kNumeric);
    EXPECT_EQ(tables[0].columns[0].type.length, 4);
    EXPECT_EQ(tables[0].columns[0].type.precision, 2);
    EXPECT_TRUE(tables[0].primary_key.empty());
}

TEST(SchemaParseContextTest, ParseSingleCharColumn) {
    std::istringstream in(R"SQLSCHEMA(
        create table foo ( bar char(20) not null );
    )SQLSCHEMA");
    SchemaParseContext spc;
    auto schema = spc.Parse(in);
    auto &tables = schema.tables;
    ASSERT_EQ(tables.size(), 1);
    EXPECT_EQ(tables[0].id, "foo");
    ASSERT_EQ(tables[0].columns.size(), 1);
    EXPECT_EQ(tables[0].columns[0].id, "bar");
    EXPECT_EQ(tables[0].columns[0].type.tclass, Type::kChar);
    EXPECT_EQ(tables[0].columns[0].type.length, 20);
    EXPECT_TRUE(tables[0].primary_key.empty());
}

TEST(SchemaParseContextTest, ParseSingleVarcharColumn) {
    std::istringstream in(R"SQLSCHEMA(
        create table foo ( bar varchar(20) not null );
    )SQLSCHEMA");
    SchemaParseContext spc;
    auto schema = spc.Parse(in);
    auto &tables = schema.tables;
    ASSERT_EQ(tables.size(), 1);
    EXPECT_EQ(tables[0].id, "foo");
    ASSERT_EQ(tables[0].columns.size(), 1);
    EXPECT_EQ(tables[0].columns[0].id, "bar");
    EXPECT_EQ(tables[0].columns[0].type.tclass, Type::kVarchar);
    EXPECT_EQ(tables[0].columns[0].type.length, 20);
    EXPECT_TRUE(tables[0].primary_key.empty());
}

TEST(SchemaParseContextTest, ParseMultipleColumns) {
    std::istringstream in(R"SQLSCHEMA(
        create table foo (
            c1 integer not null,
            c2 timestamp not null,
            c3 numeric(4, 2) not null,
            c4 char(20) not null,
            c5 varchar(20) not null
        );
    )SQLSCHEMA");
    SchemaParseContext spc;
    auto schema = spc.Parse(in);
    auto &tables = schema.tables;
    ASSERT_EQ(tables.size(), 1);
    EXPECT_EQ(tables[0].id, "foo");
    ASSERT_EQ(tables[0].columns.size(), 5);
    EXPECT_EQ(tables[0].columns[0].id, "c1");
    EXPECT_EQ(tables[0].columns[2].id, "c3");
    EXPECT_EQ(tables[0].columns[3].id, "c4");
    EXPECT_EQ(tables[0].columns[4].id, "c5");
    EXPECT_EQ(tables[0].columns[0].type.tclass, Type::kInteger);
    EXPECT_EQ(tables[0].columns[1].type.tclass, Type::kTimestamp);
    EXPECT_EQ(tables[0].columns[2].type.tclass, Type::kNumeric);
    EXPECT_EQ(tables[0].columns[3].type.tclass, Type::kChar);
    EXPECT_EQ(tables[0].columns[4].type.tclass, Type::kVarchar);
    EXPECT_EQ(tables[0].columns[2].type.length, 4);
    EXPECT_EQ(tables[0].columns[2].type.precision, 2);
    EXPECT_EQ(tables[0].columns[3].type.length, 20);
    EXPECT_EQ(tables[0].columns[4].type.length, 20);
    EXPECT_TRUE(tables[0].primary_key.empty());
}

TEST(SchemaParseContextTest, ParseMultipleTables) {
    std::istringstream in(R"SQLSCHEMA(
        create table foo (
            f1 integer not null,
            f2 timestamp not null,
            f3 numeric(4, 2) not null,
            f4 char(20) not null,
            f5 varchar(20) not null
        );

        create table bar (
            b1 integer not null,
            b2 timestamp not null,
            b3 numeric(4, 2) not null,
            b4 char(20) not null,
            b5 varchar(20) not null
        );
    )SQLSCHEMA");
    SchemaParseContext spc;
    auto schema = spc.Parse(in);
    auto &tables = schema.tables;
    ASSERT_EQ(tables.size(), 2);
    EXPECT_EQ(tables[0].id, "foo");
    ASSERT_EQ(tables[0].columns.size(), 5);
    EXPECT_EQ(tables[0].columns[0].id, "f1");
    EXPECT_EQ(tables[0].columns[2].id, "f3");
    EXPECT_EQ(tables[0].columns[3].id, "f4");
    EXPECT_EQ(tables[0].columns[4].id, "f5");
    EXPECT_EQ(tables[0].columns[0].type.tclass, Type::kInteger);
    EXPECT_EQ(tables[0].columns[1].type.tclass, Type::kTimestamp);
    EXPECT_EQ(tables[0].columns[2].type.tclass, Type::kNumeric);
    EXPECT_EQ(tables[0].columns[3].type.tclass, Type::kChar);
    EXPECT_EQ(tables[0].columns[4].type.tclass, Type::kVarchar);
    EXPECT_EQ(tables[0].columns[2].type.length, 4);
    EXPECT_EQ(tables[0].columns[2].type.precision, 2);
    EXPECT_EQ(tables[0].columns[3].type.length, 20);
    EXPECT_EQ(tables[0].columns[4].type.length, 20);
    EXPECT_TRUE(tables[0].primary_key.empty());
    EXPECT_EQ(tables[1].id, "bar");
    ASSERT_EQ(tables[1].columns.size(), 5);
    EXPECT_EQ(tables[1].columns[0].id, "b1");
    EXPECT_EQ(tables[1].columns[2].id, "b3");
    EXPECT_EQ(tables[1].columns[3].id, "b4");
    EXPECT_EQ(tables[1].columns[4].id, "b5");
    EXPECT_EQ(tables[1].columns[0].type.tclass, Type::kInteger);
    EXPECT_EQ(tables[1].columns[1].type.tclass, Type::kTimestamp);
    EXPECT_EQ(tables[1].columns[2].type.tclass, Type::kNumeric);
    EXPECT_EQ(tables[1].columns[3].type.tclass, Type::kChar);
    EXPECT_EQ(tables[1].columns[4].type.tclass, Type::kVarchar);
    EXPECT_EQ(tables[1].columns[2].type.length, 4);
    EXPECT_EQ(tables[1].columns[2].type.precision, 2);
    EXPECT_EQ(tables[1].columns[3].type.length, 20);
    EXPECT_EQ(tables[1].columns[4].type.length, 20);
    EXPECT_TRUE(tables[1].primary_key.empty());
}

TEST(SchemaParseContextTest, ParsePrimaryKey) {
    std::istringstream in(R"SQLSCHEMA(
        create table foo (
            c1 integer not null,
            c2 timestamp not null,
            c3 numeric(4, 2) not null,
            c4 char(20) not null,
            c5 varchar(20) not null,
            primary key (c1)
        );
    )SQLSCHEMA");
    SchemaParseContext spc;
    auto schema = spc.Parse(in);
    auto &tables = schema.tables;
    ASSERT_EQ(tables.size(), 1);
    EXPECT_EQ(tables[0].id, "foo");
    ASSERT_EQ(tables[0].columns.size(), 5);
    ASSERT_EQ(tables[0].primary_key.size(), 1);
    EXPECT_EQ(tables[0].primary_key[0], ("c1"));
}

TEST(SchemaParseContextTest, ParseComposedPrimaryKey) {
    std::istringstream in(R"SQLSCHEMA(
        create table foo (
            c1 integer not null,
            c2 timestamp not null,
            c3 numeric(4, 2) not null,
            c4 char(20) not null,
            c5 varchar(20) not null,
            primary key (c1, c2)
        );
    )SQLSCHEMA");
    SchemaParseContext spc;
    auto schema = spc.Parse(in);
    auto &tables = schema.tables;
    ASSERT_EQ(tables.size(), 1);
    EXPECT_EQ(tables[0].id, "foo");
    ASSERT_EQ(tables[0].columns.size(), 5);
    ASSERT_EQ(tables[0].primary_key.size(), 2);
    EXPECT_EQ(tables[0].primary_key[0], "c1");
    EXPECT_EQ(tables[0].primary_key[1], "c2");
}

TEST(SchemaParseContextTest, ParseSingleIndex) {
    std::istringstream in(R"SQLSCHEMA(
        create table foo (
            c1 integer not null,
            c2 timestamp not null,
            c3 numeric(4, 2) not null,
            c4 char(20) not null,
            c5 varchar(20) not null
        );

        create index foo_idx on foo (c2, c3, c5);
    )SQLSCHEMA");
    SchemaParseContext spc;
    auto schema = spc.Parse(in);
    auto &indexes = schema.indexes;
    ASSERT_EQ(indexes.size(), 1);
    EXPECT_EQ(indexes[0].id, "foo_idx");
    EXPECT_EQ(indexes[0].table_id, "foo");
    ASSERT_EQ(indexes[0].columns.size(), 3);
    EXPECT_EQ(indexes[0].columns[0], "c2");
    EXPECT_EQ(indexes[0].columns[1], "c3");
    EXPECT_EQ(indexes[0].columns[2], "c5");
}

TEST(SchemaParseContextTest, ParseSingleIndexWithIndexType) {
    std::istringstream in(R"SQLSCHEMA(
        create table foo (
            c1 integer not null,
            c2 timestamp not null,
            c3 numeric(4, 2) not null,
            c4 char(20) not null,
            c5 varchar(20) not null
        );

        create index foo_idx on foo (c2, c3, c5) with (index_type = btree_map);
    )SQLSCHEMA");
    SchemaParseContext spc;
    auto schema = spc.Parse(in);
    auto &indexes = schema.indexes;
    ASSERT_EQ(indexes.size(), 1);
    EXPECT_EQ(indexes[0].id, "foo_idx");
    EXPECT_EQ(indexes[0].table_id, "foo");
    EXPECT_EQ(indexes[0].index_type, IndexType::kSTXMap);
    ASSERT_EQ(indexes[0].columns.size(), 3);
    EXPECT_EQ(indexes[0].columns[0], "c2");
    EXPECT_EQ(indexes[0].columns[1], "c3");
    EXPECT_EQ(indexes[0].columns[2], "c5");
}

TEST(SchemaParseContextTest, ParseMultipleIndexes) {
    std::istringstream in(R"SQLSCHEMA(
        create table foo (
            c1 integer not null,
            c2 timestamp not null,
            c3 numeric(4, 2) not null,
            c4 char(20) not null,
            c5 varchar(20) not null
        );

        create index foo_idx1 on foo (c2, c3, c5);
        create index foo_idx2 on foo (c1, c2, c4);
    )SQLSCHEMA");
    SchemaParseContext spc;
    auto schema = spc.Parse(in);
    auto &tables = schema.tables;
    auto &indexes = schema.indexes;
    ASSERT_EQ(indexes.size(), 2);
    ASSERT_EQ(indexes[0].id, "foo_idx1");
    EXPECT_EQ(indexes[0].table_id, "foo");
    ASSERT_EQ(indexes[0].columns.size(), 3);
    EXPECT_EQ(indexes[0].columns[0], "c2");
    EXPECT_EQ(indexes[0].columns[1], "c3");
    EXPECT_EQ(indexes[0].columns[2], "c5");
    // EXPECT_EQ(indexes[0].columns[0].type.tclass, tables[0].columns[1].type.tclass);
    // EXPECT_EQ(indexes[0].columns[1].type.tclass, tables[0].columns[2].type.tclass);
    // EXPECT_EQ(indexes[0].columns[2].type.tclass, tables[0].columns[4].type.tclass);
    EXPECT_EQ(indexes[1].id, "foo_idx2");
    EXPECT_EQ(indexes[1].table_id, "foo");
    ASSERT_EQ(indexes[1].columns.size(), 3);
    EXPECT_EQ(indexes[1].columns[0], "c1");
    EXPECT_EQ(indexes[1].columns[1], "c2");
    EXPECT_EQ(indexes[1].columns[2], "c4");
    // EXPECT_EQ(indexes[1].columns[0].type.tclass, tables[0].columns[0].type.tclass);
    // EXPECT_EQ(indexes[1].columns[1].type.tclass, tables[0].columns[1].type.tclass);
    // EXPECT_EQ(indexes[1].columns[2].type.tclass, tables[0].columns[3].type.tclass);
}

}  // namespace

int main(int argc, char *argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

