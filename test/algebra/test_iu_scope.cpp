#include "algebra/IU.h"
#include "algebra/IUScope.h"
#include "algebra/suboperators/Copy.h"
#include "codegen/Type.h"
#include "gtest/gtest.h"
#include <set>

namespace inkfuse {

namespace {

TEST(test_iu_scope, basic_1) {
   IUScope scope(nullptr, 0);
   EXPECT_EQ(scope.getFilterIU(), nullptr);
   EXPECT_EQ(scope.getIdThisScope(), 0);
   IU copy_iu(IR::Void::build(), "copy_iu");
   auto copy_op = Copy::build(copy_iu);
   scope.registerIU(copy_iu, *copy_op);
   EXPECT_TRUE(scope.exists(copy_iu));
   EXPECT_EQ(&scope.getProducer(copy_iu), copy_op.get());
   auto copy_id = scope.getScopeId(copy_iu);
   EXPECT_EQ(copy_id, 0);
   auto ius = scope.getIUs();
   EXPECT_EQ(ius.size(), 1);
   EXPECT_EQ(ius[0].iu, &copy_iu);
   EXPECT_EQ(ius[0].source_scope, 0);
}

TEST(test_iu_scope, basic_2) {
   IU filter(IR::Void::build(), "test_iu");
   IUScope scope(&filter, 0);
   EXPECT_EQ(scope.getFilterIU(), &filter);
   EXPECT_EQ(scope.getIdThisScope(), 0);
   IU copy_iu(IR::Void::build(), "copy_iu");
   auto copy_op = Copy::build(copy_iu);
   scope.registerIU(copy_iu, *copy_op);
   EXPECT_TRUE(scope.exists(copy_iu));
   EXPECT_EQ(&scope.getProducer(copy_iu), copy_op.get());
   auto copy_id = scope.getScopeId(copy_iu);
   EXPECT_EQ(copy_id, 0);
   auto ius = scope.getIUs();
   EXPECT_EQ(ius.size(), 1);
   EXPECT_EQ(ius[0].iu, &copy_iu);
   EXPECT_EQ(ius[0].source_scope, 0);
}

TEST(test_iu_scope, errors) {
   IUScope scope(nullptr, 0);
   IU copy_iu(IR::Void::build(), "copy_iu");
   auto copy_op = Copy::build(copy_iu);
   EXPECT_FALSE(scope.exists(copy_iu));
   EXPECT_ANY_THROW(scope.getProducer(copy_iu));
   EXPECT_ANY_THROW(scope.getScopeId(copy_iu));
   EXPECT_EQ(scope.getIUs().size(), 0);
}

TEST(test_iu_scope, identifiers) {
   IUScope scope(nullptr, 0);
   std::vector<IU> ius;
   for (uint64_t k = 0; k < 100; ++k) {
      const auto& iu = ius.emplace_back(IR::Void::build(), "copy_iu");
      auto copy_op = Copy::build(iu);
      scope.registerIU(iu, *copy_op);
      auto id = scope.getScopeId(iu);
      EXPECT_EQ(id, 0);
   }
   EXPECT_EQ(scope.getIUs().size(), 100);
}

TEST(test_iu_scope, rewire) {
   IU filter(IR::Void::build(), "test_iu");
   IUScope scope(&filter, 0);
   EXPECT_EQ(scope.getFilterIU(), &filter);
   EXPECT_EQ(scope.getIdThisScope(), 0);
   auto rewired = IUScope::rewire(nullptr, scope);
   EXPECT_EQ(rewired.getFilterIU(), nullptr);
   EXPECT_EQ(rewired.getIdThisScope(), 1);
}

TEST(test_iu_scope, retain) {
   IU filter(IR::Void::build(), "test_iu");
   IUScope scope(&filter, 0);
   EXPECT_EQ(scope.getFilterIU(), &filter);
   EXPECT_EQ(scope.getIdThisScope(), 0);
   IU copy_iu(IR::Void::build(), "copy_iu");
   auto copy_op = Copy::build(copy_iu);
   scope.registerIU(copy_iu, *copy_op);
   IU copy_iu_2(IR::Void::build(), "copy_iu_2");
   auto new_parent = Copy::build(copy_iu_2);
   IUScope retain = IUScope::retain(nullptr, scope, *new_parent, {&copy_iu});
   EXPECT_EQ(retain.getFilterIU(), nullptr);
   EXPECT_EQ(&retain.getProducer(copy_iu), new_parent.get());
   EXPECT_NO_THROW(retain.getScopeId(copy_iu));
   EXPECT_EQ(retain.getIUs().size(), 1);
   EXPECT_EQ(retain.getIdThisScope(), 1);
   IU new_iu_new_scope(IR::Void::build(), "copy_iu_3");
   retain.registerIU(new_iu_new_scope, *new_parent);
   EXPECT_EQ(retain.getScopeId(copy_iu), 0);
   EXPECT_EQ(retain.getScopeId(new_iu_new_scope), 1);
}

}

}