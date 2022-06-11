#include "gtest/gtest.h"
#include "algebra/Pipeline.h"
#include "algebra/suboperators/Suboperator.h"
#include <deque>

namespace inkfuse {

namespace {

template <bool source = false, bool sink = false, bool incoming_strong = false, bool outgoing_strong = false>
struct DynamicSuboperator : Suboperator {

   std::deque<IU> out_ius;

   bool isSink() const override { return sink; }
   bool isSource() const override { return source; }
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


/// Types of operators depending on source/sink and link property.
using ComputeDyn = DynamicSuboperator<false, false, false, false>;
using SourceDyn = DynamicSuboperator<true, false, false, true>;
using SinkDyn = DynamicSuboperator<false, true, false, false>;
using ScopeDyn = DynamicSuboperator<false, false, false, true>;
using ScopeConsumerDyn = DynamicSuboperator<false, false, true, false>;

struct RepipeF : public ::testing::Test {

   void setUpSourceIUs(size_t count) {
      for (size_t k = 0; k < count; ++k) {
         const auto& iu = source_ius.emplace_back(IR::UnsignedInt::build(4));
      }
   };

   std::deque<IU> source_ius;
   Pipeline pipe;
};

TEST_F(RepipeF, simpe_repipe_closed) {
   // Single source.
   auto source = build<SourceDyn>({}, 1);
   auto& source_ius = source->getIUs();
   // One consumer.
   auto op = build<ComputeDyn>({source_ius[0]}, 1);
   auto& op_ius = op->getIUs();
   // One sink.
   auto sink = build<SinkDyn>({op_ius[0]}, 0);

   pipe.attachSuboperator(source);
   pipe.attachSuboperator(op);
   pipe.attachSuboperator(sink);

   // We should have a fourth sink consuming source_ius[0].
   auto repiped_all = pipe.repipeAll(0, 3);
   EXPECT_EQ(repiped_all->getSubops().size(), 4);
   // We should materialize op_ius twice now.
   const auto& repiped_all_sources = repiped_all->getSubops()[3]->getSourceIUs();
   EXPECT_EQ(repiped_all_sources.size(), 1);
   EXPECT_EQ(repiped_all_sources[0], op_ius[0]);

   auto repiped_req = pipe.repipeRequired(0, 3);
   EXPECT_EQ(repiped_req->getSubops().size(), 3);
}

TEST_F(RepipeF, deep_repipe_open) {
   // Single source.
   auto source = build<SourceDyn>({}, 1);
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
}

}
