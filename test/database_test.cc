// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------

#include <sstream>
#include "imlab/test/data.h"
#include "imlab/database.h"
#include "gtest/gtest.h"

namespace {

TEST(DatabaseTest, LoadWarehouse) {
    imlab::Database db;
    std::stringstream in(imlab_test::kTestWarehouse);
    db.LoadWarehouse(in);
    ASSERT_EQ(db.Size<imlab::tpcc::kWarehouse>(), 5);
}

TEST(DatabaseTest, LoadDistrict) {
    imlab::Database db;
    std::stringstream in(imlab_test::kTestDistrict);
    db.LoadDistrict(in);
    ASSERT_EQ(db.Size<imlab::tpcc::kDistrict>(), 4);
}

TEST(DatabaseTest, LoadHistory) {
    imlab::Database db;
    std::stringstream in(imlab_test::kTestHistory);
    db.LoadHistory(in);
    ASSERT_EQ(db.Size<imlab::tpcc::kHistory>(), 4);
}

TEST(DatabaseTest, LoadItem) {
    imlab::Database db;
    std::stringstream in(imlab_test::kTestItem);
    db.LoadItem(in);
    ASSERT_EQ(db.Size<imlab::tpcc::kItem>(), 4);
}

TEST(DatabaseTest, LoadNewOrder) {
    imlab::Database db;
    std::stringstream in(imlab_test::kTestNewOrder);
    db.LoadNewOrder(in);
    ASSERT_EQ(db.Size<imlab::tpcc::kNewOrder>(), 4);
}

TEST(DatabaseTest, LoadOrder) {
    imlab::Database db;
    std::stringstream in(imlab_test::kTestOrder);
    db.LoadOrder(in);
    ASSERT_EQ(db.Size<imlab::tpcc::kOrder>(), 4);
}

TEST(DatabaseTest, LoadOrderLine) {
    imlab::Database db;
    std::stringstream in(imlab_test::kTestOrderLine);
    db.LoadOrderLine(in);
    ASSERT_EQ(db.Size<imlab::tpcc::kOrderLine>(), 4);
}

TEST(DatabaseTest, LoadStock) {
    imlab::Database db;
    std::stringstream in(imlab_test::kTestStock);
    db.LoadStock(in);
    ASSERT_EQ(db.Size<imlab::tpcc::kStock>(), 4);
}

TEST(DatabaseTest, LoadCustomer) {
    imlab::Database db;
    std::stringstream in(imlab_test::kTestCustomer);
    db.LoadCustomer(in);
    ASSERT_EQ(db.Size<imlab::tpcc::kCustomer>(), 2);
}

}  // namespace
