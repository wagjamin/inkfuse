
#include <sstream>
#include "query_parse_context.h"
#include "query_ast.h"
#include "gtest/gtest.h"

namespace {

    using namespace imlab::queryc;

    using Triple = std::tuple<std::string, std::string, PredType>;

    TEST(SchemaParseContextTest, ParseEmptyString) {
        std::istringstream in("");
        QueryParseContext qpc;
        EXPECT_ANY_THROW(qpc.Parse(in));
    }

    TEST(SchemaParseContextTest, ParseSimpleNoWhere) {
        std::istringstream in("SELECT a FROM b;");
        QueryParseContext qpc;
        auto result = qpc.Parse(in);
        ASSERT_EQ(result.select_list.size(), 1);
        ASSERT_EQ(result.from_list.size(), 1);
        ASSERT_EQ(result.where_list.size(), 0);
    }

    TEST(SchemaParseContextTest, ParseComplexNoWhere) {
        std::istringstream in("SELECT a, c FROM b, d;");
        QueryParseContext qpc;
        auto result = qpc.Parse(in);
        ASSERT_EQ(result.select_list.size(), 2);
        ASSERT_EQ(result.from_list.size(), 2);
        ASSERT_EQ(result.where_list.size(), 0);
        ASSERT_EQ(result.select_list[0], "a");
        ASSERT_EQ(result.select_list[1], "c");
        ASSERT_EQ(result.from_list[0], "b");
        ASSERT_EQ(result.from_list[1], "d");
    }

    TEST(SchemaParseContextTest, ParseSimpleWhereInt) {
        std::istringstream in("SELECT a FROM b WHERE c = 1;");
        QueryParseContext qpc;
        auto result = qpc.Parse(in);
        ASSERT_EQ(result.select_list.size(), 1);
        ASSERT_EQ(result.from_list.size(), 1);
        ASSERT_EQ(result.where_list.size(), 1);
        ASSERT_EQ(result.select_list[0], "a");
        ASSERT_EQ(result.from_list[0], "b");
        ASSERT_EQ(result.where_list[0], std::make_tuple("c", "1", PredType::IntConstant));
    }

    TEST(SchemaParseContextTest, ParseSimpleWhereString) {
        std::istringstream in("SELECT a FROM b WHERE c = 'HelloWorld';");
        QueryParseContext qpc;
        auto result = qpc.Parse(in);
        ASSERT_EQ(result.select_list.size(), 1);
        ASSERT_EQ(result.from_list.size(), 1);
        ASSERT_EQ(result.where_list.size(), 1);
        ASSERT_EQ(result.select_list[0], "a");
        ASSERT_EQ(result.from_list[0], "b");
        ASSERT_EQ(result.where_list[0], std::make_tuple("c", "HelloWorld", PredType::StringConstant));
    }

    TEST(SchemaParseContextTest, ParseSimpleWhereColRef) {
        std::istringstream in("SELECT a FROM b WHERE c = d;");
        QueryParseContext qpc;
        auto result = qpc.Parse(in);
        ASSERT_EQ(result.select_list.size(), 1);
        ASSERT_EQ(result.from_list.size(), 1);
        ASSERT_EQ(result.where_list.size(), 1);
        ASSERT_EQ(result.select_list[0], "a");
        ASSERT_EQ(result.from_list[0], "b");
        ASSERT_EQ(result.where_list[0], std::make_tuple("c", "d", PredType::ColumnRef));
    }

    TEST(SchemaParseContextTest, ParseComplexWhereColRef) {
        std::istringstream in("SELECT a FROM b WHERE c = d AND e = 'f' AND g = 1;");
        QueryParseContext qpc;
        auto result = qpc.Parse(in);
        ASSERT_EQ(result.select_list.size(), 1);
        ASSERT_EQ(result.from_list.size(), 1);
        ASSERT_EQ(result.where_list.size(), 3);
        ASSERT_EQ(result.select_list[0], "a");
        ASSERT_EQ(result.from_list[0], "b");
        ASSERT_EQ(result.where_list[0], std::make_tuple("c", "d", PredType::ColumnRef));
        ASSERT_EQ(result.where_list[1], std::make_tuple("e", "f", PredType::StringConstant));
        ASSERT_EQ(result.where_list[2], std::make_tuple("g", "1", PredType::IntConstant));
    }
}

int main(int argc, char *argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
