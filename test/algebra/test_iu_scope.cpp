#include "algebra/IU.h"
#include "algebra/IUScope.h"
#include "algebra/suboperators/Copy.h"
#include "codegen/Type.h"
#include "gtest/gtest.h"
#include <set>

namespace inkfuse {

namespace {

TEST(test_iu_scope, basic_1) {
   IUScope scope(nullptr);
   EXPECT_EQ(scope.getFilterIU(), nullptr);
   IU copy_iu(IR::Void::build(), "copy_iu");
   auto copy_op = Copy::build(copy_iu);
   scope.registerIU(copy_iu, *copy_op);
   EXPECT_TRUE(scope.exists(copy_iu));
   EXPECT_EQ(&scope.getProducer(copy_iu), copy_op.get());
   auto copy_id = scope.getId(copy_iu);
   auto ius = scope.getIUs();
   EXPECT_EQ(ius.size(), 1);
   EXPECT_EQ(ius[0].iu, &copy_iu);
   EXPECT_EQ(ius[0].id, copy_id);
}

TEST(test_iu_scope, basic_2) {
   IU filter(IR::Void::build(), "test_iu");
   IUScope scope(&filter);
   EXPECT_EQ(scope.getFilterIU(), &filter);
   IU copy_iu(IR::Void::build(), "copy_iu");
   auto copy_op = Copy::build(copy_iu);
   scope.registerIU(copy_iu, *copy_op);
   EXPECT_TRUE(scope.exists(copy_iu));
   EXPECT_EQ(&scope.getProducer(copy_iu), copy_op.get());
   auto copy_id = scope.getId(copy_iu);
   auto ius = scope.getIUs();
   EXPECT_EQ(ius.size(), 1);
   EXPECT_EQ(ius[0].iu, &copy_iu);
   EXPECT_EQ(ius[0].id, copy_id);
}

TEST(test_iu_scope, errors) {
   IUScope scope(nullptr);
   IU copy_iu(IR::Void::build(), "copy_iu");
   auto copy_op = Copy::build(copy_iu);
   EXPECT_FALSE(scope.exists(copy_iu));
   EXPECT_ANY_THROW(scope.getProducer(copy_iu));
   EXPECT_ANY_THROW(scope.getId(copy_iu));
   EXPECT_EQ(scope.getIUs().size(), 0);
}

TEST(test_iu_scope, identifiers) {
   IUScope scope(nullptr);
   std::set<uint64_t> iu_ids;
   std::vector<IU> ius;
   for (uint64_t k = 0; k < 100; ++k) {
      const auto& iu = ius.emplace_back(IR::Void::build(), "copy_iu");
      auto copy_op = Copy::build(iu);
      scope.registerIU(iu, *copy_op);
      auto id = scope.getId(iu);
      EXPECT_EQ(iu_ids.count(id), 0);
      iu_ids.insert(id);
   }
   EXPECT_EQ(scope.getIUs().size(), 100);
}

TEST(test_iu_scope, rewire) {
   IU filter(IR::Void::build(), "test_iu");
   auto scope = IUScope::rewire(&filter);
   EXPECT_EQ(scope.getFilterIU(), &filter);
}

TEST(test_iu_scope, retain) {
   IU filter(IR::Void::build(), "test_iu");
   IUScope scope(&filter);
   EXPECT_EQ(scope.getFilterIU(), &filter);
   IU copy_iu(IR::Void::build(), "copy_iu");
   auto copy_op = Copy::build(copy_iu);
   scope.registerIU(copy_iu, *copy_op);
   IUScope retain = IUScope::retain(scope, nullptr, {&copy_iu});
   EXPECT_EQ(retain.getFilterIU(), nullptr);
   EXPECT_EQ(&retain.getProducer(copy_iu), copy_op.get());
   EXPECT_NO_THROW(retain.getId(copy_iu));
   EXPECT_EQ(retain.getIUs().size(), 1);
}

}

}