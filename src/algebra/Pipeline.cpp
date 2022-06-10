#include "algebra/Pipeline.h"
#include "algebra/suboperators/Suboperator.h"
#include "algebra/suboperators/sinks/FuseChunkSink.h"
#include "algebra/suboperators/sources/FuseChunkSource.h"
#include <algorithm>

namespace inkfuse {

std::unique_ptr<Pipeline> Pipeline::repipeAll(size_t start, size_t end) const {
   // Find everything we provide that's not from strong output links.
   std::unordered_set<const IU*> out_provided;
   std::for_each(
      suboperators.begin() + start,
      suboperators.begin() + end,
      [&](const SuboperatorArc& subop) {
         if (!subop->outgoingStrongLinks()) {
            for (auto iu : subop->getIUs()) {
               if (!dynamic_cast<IR::Void*>(iu->type.get())) {
                  // We do not add void IUs as these present pseudo IU to properly connect the graph.
                  out_provided.insert(iu);
               }
            }
         }
      });

   return repipe(start, end, out_provided);
}

std::unique_ptr<Pipeline> Pipeline::repipeRequired(size_t start, size_t end) const {
   // Find output IUs we provide.
   std::unordered_set<const IU*> out_provided;
   std::for_each(
      suboperators.begin() + start,
      suboperators.begin() + end,
      [&](const SuboperatorArc& subop) {
         for (auto iu : subop->getIUs()) {
            out_provided.insert(iu);
         }
      });

   // And figure out which of those will be needed by other sub-operators down the line.
   std::unordered_set<const IU*> out_required;
   std::for_each(
      suboperators.begin() + end,
      suboperators.end(),
      [&](const SuboperatorArc& subop) {
         for (auto iu : subop->getSourceIUs()) {
            if (!out_provided.count(iu)) {
               out_required.insert(iu);
            }
         }
      });

   return repipe(start, end, out_required);
}

std::unique_ptr<Pipeline> Pipeline::repipe(size_t start, size_t end, const std::unordered_set<const IU*>& materialize) const {
   auto new_pipe = std::make_unique<Pipeline>();
   if (start == end) {
      // We are creating an empty pipe, return.
      return new_pipe;
   }

   const auto& first = suboperators[start];
   const auto& last = suboperators[end - 1];

   // Find out which IUs we need to access that are not provided by one of the suboperators in [start, end[.
   std::unordered_set<const IU*> in_provided;
   std::unordered_set<const IU*> in_required;
   std::for_each(
      suboperators.begin() + start,
      suboperators.begin() + end,
      [&](const SuboperatorArc& subop) {
         for (auto iu : subop->getIUs()) {
            in_provided.insert(iu);
            assert(!in_required.count(iu));
         }
         for (auto iu : subop->getSourceIUs()) {
            if (!in_provided.count(iu)) {
               in_required.insert(iu);
            }
         }
      });

   if (!in_required.empty()) {
      // Attach the IU proving operators.
      auto try_provider = tryGetProvider(**in_required.begin());
      if (try_provider && try_provider->isSource()) {
         // If the provider was actually a source, it has to be at index zero. Retain it.

         new_pipe->attachSuboperator(suboperators[0]);
      } else {
         // Otherwise, build a FuseChunkSource for those that are not driven by an existing provider.
         // First we build the driver.
         auto& driver = new_pipe->attachSuboperator(FuseChunkSourceDriver::build());
         auto& driver_iu = **driver.getIUs().begin();
         // And the IU providers.
         for (auto iu : in_required) {
            new_pipe->attachSuboperator(FuseChunkSourceIUProvider::build(driver_iu, *iu));
         }
      }
   }

   if (first->incomingStrongLinks()) {
      // The suboperator has incoming strong links, meaning it cannot be separated from its input
      // suboperator. We have to attac the source of the strong link.
      // Note that at the moment, the length of a strong link chain can be at most one hop.
      const auto& input = graph.incoming_edges.at(first.get());
      assert(input.size() == 1);
      auto strong_op = std::find_if(suboperators.begin(), suboperators.end(), [&](const SuboperatorArc& elem) {
         return elem.get() == input[0];
      });
      assert(strong_op != suboperators.end());
      new_pipe->attachSuboperator(*strong_op);
   }
   // Copy all intermediate operators.
   std::for_each(
      suboperators.begin() + start,
      suboperators.begin() + end,
      [&](const SuboperatorArc& op) {
         new_pipe->attachSuboperator(op);
      });

   if (last->outgoingStrongLinks()) {
      // The suboperator has outgoing strong links, meaning it cannot be separated from its consuming
      // suboperator. We have move the end to contain all consumers.
      // Note that at the moment, the length of this chain can be at most one hop.
      const auto& output = graph.outgoing_edges.at(last.get());
      assert(input.size() == 1);
      auto strong_op = std::find_if(suboperators.begin(), suboperators.end(), [&](const SuboperatorArc& elem) {
         return elem.get() == output[0];
      });
      assert(strong_op != suboperators.end());
      new_pipe->attachSuboperator(*strong_op);
   }

   for (auto& out_iu : materialize) {
      // And build fuse chunk sinks for those.
      new_pipe->attachSuboperator(
         FuseChunkSink::build(nullptr, *out_iu));
   }

   return new_pipe;
}

const std::vector<Suboperator*>& Pipeline::getConsumers(Suboperator& subop) const {
   return graph.outgoing_edges.at(&subop);
}

const std::vector<Suboperator*>& Pipeline::getProducers(Suboperator& subop) const {
   return graph.incoming_edges.at(&subop);
}

Suboperator& Pipeline::getProvider(const IU& iu) const {
   return *iu_providers.at(&iu);
}

Suboperator* Pipeline::tryGetProvider(const IU& iu) const {
   if (iu_providers.count(&iu)) {
      return iu_providers.at(&iu);
   }
   return nullptr;
}

Suboperator& Pipeline::attachSuboperator(SuboperatorArc subop) {
   // Update the pipeline graph.
   for (const auto& depends : subop->getSourceIUs()) {
      // Resolve input.
      if (auto provider = tryGetProvider(*depends)) {
         // Add edges.
         graph.outgoing_edges[provider].push_back(subop.get());
         graph.incoming_edges[subop.get()].push_back(provider);
      }
   }
   // Register newly produced IUs within the current scope.
   for (auto iu : subop->getIUs()) {
      // Add the produced IU to the (potentially) new scope.
      iu_providers[iu] = subop.get();
   }
   suboperators.push_back(std::move(subop));
   return *suboperators.back();
}

const std::vector<SuboperatorArc>& Pipeline::getSubops() const {
   return suboperators;
}

Pipeline& PipelineDAG::getCurrentPipeline() const {
   assert(!pipelines.empty());
   return *pipelines.back();
}

Pipeline& PipelineDAG::buildNewPipeline() {
   pipelines.push_back(std::make_unique<Pipeline>());
   return *pipelines.back();
}

const std::vector<PipelinePtr>& PipelineDAG::getPipelines() const {
   return pipelines;
}

}