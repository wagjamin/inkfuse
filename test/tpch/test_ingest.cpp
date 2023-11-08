#include "common/TPCH.h"
#include "common/Helpers.h"
#include "gtest/gtest.h"
#include <cstring>

namespace inkfuse {

namespace {

void testLineitem(Schema& schema) {
   auto& table = schema["lineitem"];
   // Every column should have 6005 rows ingested.
   for (size_t col_idx = 0; col_idx < table->columnCount(); ++col_idx) {
      auto [_, col] = table->getColumn(col_idx);
      EXPECT_EQ(col.length(), 6005);
   }
   {
      // l_returnflag in {A, N, R}
      std::set<char> flags{'A', 'N', 'R'};
      auto& l_returnflag = table->getColumn("l_returnflag");
      auto l_returnflag_data = l_returnflag.getRawData();
      for (size_t row = 0; row < 6005; ++row) {
         EXPECT_TRUE(flags.contains(l_returnflag_data[row]));
      }
   }
   {
      // l_orderkey increasing from 1 to 5988
      auto& l_orderkey = table->getColumn("l_orderkey");
      auto l_orderkey_data = reinterpret_cast<int32_t*>(l_orderkey.getRawData());
      for (size_t row = 0; row < 6005; ++row) {
         EXPECT_GE(l_orderkey_data[row], 1);
         EXPECT_LE(l_orderkey_data[row], 5988);
         if (row > 0) {
            EXPECT_GE(l_orderkey_data[row], l_orderkey_data[row - 1]);
         }
      }
   }
   {
      // l_shipdate in the 90s
      auto& l_shipdate = table->getColumn("l_shipdate");
      auto l_shipdate_data = reinterpret_cast<int32_t*>(l_shipdate.getRawData());
      auto min = helpers::dateStrToInt("1990-1-1");
      auto max = helpers::dateStrToInt("1999-12-31");
      EXPECT_GE(min, 365 * 20);
      EXPECT_LE(min, 366 * 30);
      for (size_t row = 0; row < 6005; ++row) {
         EXPECT_GE(l_shipdate_data[row], min);
         EXPECT_LE(l_shipdate_data[row], max);
      }
   }
}

TEST(test_ingest, testdata) {
   // Load the TPC-H schema & data.
   auto schema = tpch::getTPCHSchema();
   // Force the files to exist during ingest.
   helpers::loadDataInto(schema, "test/tpch/testdata", true);
   // Spot check the ingested tables.
   testLineitem(schema);
}

}

}
