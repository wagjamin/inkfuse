#include "gtest/gtest.h"
#include "algebra/Pipeline.h"
#include "algebra/suboperators/Suboperator.h"
#include "algebra/suboperators/sources/FuseChunkSource.h"
#include "algebra/suboperators/sinks/FuseChunkSink.h"
#include <deque>

namespace inkfuse {

namespace {

template <bool incoming_strong = false, bool outgoing_strong = false>
struct DynamicSuboperator : Suboperator {

   std::deque<IU> out_ius;

   bool incomingStrongLinks() const override { return incoming_strong; }
   bool outgoingStrongLinks() const override { return outgoing_strong; }

   std::string id() const override { return "DynamicSuboperator"; };

   DynamicSuboperator(std::vector<const IU*> incoming, size_t outgoing): Suboperator(nullptr, {}, std::move(incoming)) {
      for (size_t k = 0; k < outgoing; ++k) {
         const auto& iu = out_ius.template emplace_back(IR::UnsignedInt::build(4));
         provided_ius.push_back(&iu);
      }
   };


};

template<class DynSub>
SuboperatorArc build(std::vector<const IU*> incoming, size_t outgoing) {
   return SuboperatorArc(new DynSub(std::move(incoming), outgoing));
};


/// Types of operators depending on link property.
using ComputeDyn = DynamicSuboperator<false, false>;
using ScopeDyn = DynamicSuboperator<false, true>;
using ScopeConsumerDyn = DynamicSuboperator<true, false>;

struct RepipeF : public ::testing::Test {

   void setUpSourceIUs(size_t count) {
      for (size_t k = 0; k < count; ++k) {
         const auto& iu = source_ius.emplace_back(IR::UnsignedInt::build(4));
      }
   };

   std::deque<IU> source_ius;
   Pipeline pipe;
};

/// Simple repipe test mirroring scanning from a managed table.
TEST_F(RepipeF, simpe_repipe_table_scan) {
   // Single source with outgoing strong link.
   auto source = build<ScopeDyn>({}, 1);
   auto& source_ius = source->getIUs();
   // One consumer.
   auto op = build<ComputeDyn>({source_ius[0]}, 1);
   auto& op_ius = op->getIUs();
   // One sink.
   auto sink = build<ComputeDyn>({op_ius[0]}, 0);

   pipe.attachSuboperator(source);
   pipe.attachSuboperator(op);
   pipe.attachSuboperator(sink);

   // There should be an extra sink materializing op_ius[0].
   auto repiped_all = pipe.repipeAll(0, 3);
   EXPECT_EQ(repiped_all->getSubops().size(), 4);
   // We should materialize op_ius twice now.
   const auto& repiped_all_sources = repiped_all->getSubops()[3]->getSourceIUs();
   EXPECT_EQ(repiped_all_sources.size(), 1);
   EXPECT_EQ(repiped_all_sources[0], op_ius[0]);

   // As there are no downstream consumers that actually need the op_iu, it should
   // not be retained during repipeRequired.
   auto repiped_req = pipe.repipeRequired(0, 3);
   EXPECT_EQ(repiped_req->getSubops().size(), 3);
}

/// Simple repipe test mirroring scanning from a hash table.
TEST_F(RepipeF, simpe_repipe_ht_scan) {
   // Single source without outgoing strong link.
   auto source = build<ComputeDyn>({}, 1);
   auto& source_ius = source->getIUs();
   // One consumer.
   auto op = build<ComputeDyn>({source_ius[0]}, 1);
   auto& op_ius = op->getIUs();
   // One sink.
   auto sink = build<ComputeDyn>({op_ius[0]}, 0);

   pipe.attachSuboperator(source);
   pipe.attachSuboperator(op);
   pipe.attachSuboperator(sink);

   // There should be two extra sinks - one materializing source_ius[0] and one materializing op_ius[0].
   auto repiped_all = pipe.repipeAll(0, 3);
   EXPECT_EQ(repiped_all->getSubops().size(), 5);
   std::unordered_set<const IU*> materialized;
   const auto& sources_source_materializer = repiped_all->getSubops()[3]->getSourceIUs();
   EXPECT_EQ(sources_source_materializer.size(), 1);
   materialized.emplace(sources_source_materializer[0]);
   const auto& sources_op_materializer = repiped_all->getSubops()[4]->getSourceIUs();
   EXPECT_EQ(sources_op_materializer.size(), 1);
   materialized.emplace(sources_op_materializer[0]);
   EXPECT_TRUE(materialized.contains(source_ius[0]));
   EXPECT_TRUE(materialized.contains(op_ius[0]));

   // As there are no downstream consumers that actually need any IU, it should
   // not be retained during repipeRequired.
   auto repiped_req = pipe.repipeRequired(0, 3);
   EXPECT_EQ(repiped_req->getSubops().size(), 3);
}

TEST_F(RepipeF, deep_repipe_open) {
   // Single table-scan style source (does not get materialized).
   auto source = build<ScopeDyn>({}, 1);
   auto& source_ius = source->getIUs();
   pipe.attachSuboperator(source);

   // One hundred consumers producing two IUs, who again have 100 consumers.
   std::deque<SuboperatorArc> followers_1;
   std::deque<SuboperatorArc> followers_2;
   std::unordered_set<const IU*> out_ius_1;
   for (size_t k = 0; k < 100; ++k) {
      auto op = build<ComputeDyn>({source_ius[0]}, 2);
      auto& op_ius = op->getIUs();
      out_ius_1.insert(op_ius[0]);
      pipe.attachSuboperator(op);
      followers_1.push_back(op);
   }
   for (size_t k = 0; k < 100; ++k) {
      auto& in_ius = followers_1[k]->getIUs();
      auto op = build<ComputeDyn>({in_ius[0], in_ius[1]}, 1);
      pipe.attachSuboperator(op);
   }

   // Nothing is required.
   auto repiped_req = pipe.repipeRequired(0, pipe.getSubops().size());
   ASSERT_EQ(repiped_req->getSubops().size(), 201);

   auto repiped_all = pipe.repipeAll(0, pipe.getSubops().size());
   // 100 * 2 IUs + 100 * 1 IU.
   ASSERT_EQ(repiped_all->getSubops().size(), 201 + 300);

   auto repiped_cust = pipe.repipe(0, pipe.getSubops().size(), out_ius_1);
   // 100 custom requests.
   ASSERT_EQ(repiped_cust->getSubops().size(), 201 + 100);
}

/// Test is repiping works on the start of a new "scope".
TEST_F(RepipeF, scope_start) {
   setUpSourceIUs(2);
   // Filter style - we filter on the first IU.
   auto scope = build<ScopeDyn>({&source_ius[0]}, 1);
   auto& scope_ius = scope->getIUs();
   // Consume this. First IU is the strong link.
   auto compute = build<ScopeConsumerDyn>({scope_ius[0], &source_ius[1]}, 1);

   pipe.attachSuboperator(scope);
   pipe.attachSuboperator(compute);

   auto verify = [&scope, &compute](Pipeline& pipe) {
     const auto& ops = pipe.getSubops();
     // Add 3 providers for the two input IUs. In the end one consumer for the compute IU.
     EXPECT_EQ(ops.size(), 6);
     // First three are providers.
     EXPECT_TRUE(dynamic_cast<FuseChunkSourceDriver*>(ops[0].get()));
     EXPECT_TRUE(dynamic_cast<FuseChunkSourceIUProvider*>(ops[1].get()));
     EXPECT_TRUE(dynamic_cast<FuseChunkSourceIUProvider*>(ops[2].get()));
     EXPECT_EQ(ops[3].get(), scope.get());
     EXPECT_EQ(ops[4].get(), compute.get());
     EXPECT_TRUE(dynamic_cast<FuseChunkSink*>(ops[5].get()));
     EXPECT_EQ(ops[5]->getSourceIUs()[0], compute->getIUs()[0]);
   };

   // As we have a strong link in the beginning, repiping should be invariant under index 0/1.
   auto repiped_all = pipe.repipeAll(0, pipe.getSubops().size());
   auto repiped_second = pipe.repipeAll(1, pipe.getSubops().size());
   verify(*repiped_all);
   verify(*repiped_second);
}
}

}
