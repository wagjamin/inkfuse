#include "algebra/Pipeline.h"
#include "algebra/suboperators/Suboperator.h"
#include "algebra/suboperators/sinks/FuseChunkSink.h"
#include "algebra/suboperators/sources/FuseChunkSource.h"
#include <algorithm>

namespace inkfuse {

std::unique_ptr<Pipeline> Pipeline::repipe(size_t start, size_t end, bool materialize_all) const {
   // TODO Currently this does not support proper scope offsetting if start is not in the first scope.
   auto new_pipe = std::make_unique<Pipeline>();
   if (start == end) {
      // We are creating an empty pipe, return.
      return new_pipe;
   }

   const auto& first = suboperators[start];
   const auto& last = suboperators[end - 1];
   if (first->incomingStrongLinks()) {
      // The suboperator has incoming strong links, meaning it cannot be separated from its input
      // suboperator. We have to move start back.
      // Note that at the moment, the length of this chain can be at most one hop.
      const auto& input = graph.incoming_edges.at(first.get());
      assert(input.size() == 1);
      auto input_it = std::find(suboperators.cbegin(), suboperators.cend(), [&](const SuboperatorArc& elem){
         return elem.get() == input[0];
      });
      start = std::distance(suboperators.cbegin(), input_it);
   }
   if (last->outgoingStrongLinks()) {
      // The suboperator has outgoing strong links, meaning it cannot be separated from its consuming
      // suboperator. We have move the end to contain all consumers.
      // Note that at the moment, the length of this chain can be at most one hop.
      // TODO
   }

   // Find out which IUs we will need in the first in_scope that are not in_provided.
   std::set<const IU*> in_provided;
   std::set<const IU*> in_required;
   std::for_each(
      suboperators.begin() + start,
      suboperators.begin() + end,
      [&](const SuboperatorArc& subop) {
        for (auto iu : subop->getIUs()) {
           in_provided.insert(iu);
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

   // Copy all intermediate operators.
   std::for_each(
      suboperators.begin() + start,
      suboperators.begin() + end,
      [&](const SuboperatorArc& op) {
         new_pipe->attachSuboperator(op);
      });

   // Find output IUs which have to be materialized.
   std::set<const IU*> out_required;
   if (materialize_all) {
      // TODO become more explicit and drop materialize_all.
      // Find all produced IUs which are not loop drivers.
      std::for_each(
         suboperators.begin() + start,
         suboperators.begin() + end,
         [&](const SuboperatorArc& subop) {
            for (auto iu : subop->getIUs()) {
               if (!subop->isSource()) {
                  out_required.insert(iu);
               }
            }
         });
   } else {
      std::set<const IU*> out_provided;
      std::for_each(
         suboperators.begin() + std::max(start, out_scope_start),
         suboperators.begin() + end,
         [&](const SuboperatorArc& subop) {
            for (auto iu : subop->getIUs()) {
               out_provided.insert(iu);
            }
         });
      // Find out which IUs we will need after this interval. Note that IUs can also be consumed
      // in followup scopes.
      // We don't have to worry about IU scoping here, as if a followup scope uses the iu, it has
      // to be updated by the producing operator in the previous scope.
      std::for_each(
         suboperators.begin() + end,
         suboperators.end(),
         [&](const SuboperatorArc& subop) {
            for (auto iu : subop->getSourceIUs()) {
               if (!subop->isSource() && out_provided.count(iu)) {
                  out_required.insert(iu);
               }
            }
         });
   }

   for (auto& out_iu : out_required) {
      // And build fuse chunk sinks for those.
      new_pipe->attachSuboperator(
         FuseChunkSink::build(nullptr, *out_iu));
   }

   return new_pipe;
}

const IUScope& Pipeline::getScope(size_t id) const {
   assert(id < scopes.size());
   return *scopes[id];
}

const std::vector<Suboperator*>& Pipeline::getConsumers(Suboperator& subop) const {
   return graph.outgoing_edges.at(&subop);
}

const std::vector<Suboperator*>& Pipeline::getProducers(Suboperator& subop) const {
   return graph.incoming_edges.at(&subop);
}

Suboperator& Pipeline::getProvider(size_t scope_idx, const IU& iu) const {
   return scopes.at(scope_idx)->getProducer(iu);
}

Suboperator* Pipeline::tryGetProvider(size_t scope_idx, const IU& iu) const {
   if (scopes.at(scope_idx)->exists(iu)) {
      return &scopes.at(scope_idx)->getProducer(iu);
   }
   return nullptr;
}

size_t Pipeline::resolveOperatorScope(const Suboperator& op, bool incoming) const {
   // Find the index of the suboperator in the topological sort.
   auto it = std::find_if(suboperators.cbegin(), suboperators.cend(), [&op](const SuboperatorArc& target) {
      return target.get() == &op;
   });
   auto idx = std::distance(suboperators.cbegin(), it);

   // Find the index of the scope of the input ius of the target operator.
   if (idx == 0) {
      // Source is a special operator which also resolves to scope 0.
      // This is fine since the source will never have input ius it needs to resolve.
      return 0;
   }
   decltype(rescope_offsets)::const_iterator it_scope;
   if (incoming) {
      it_scope = std::lower_bound(rescope_offsets.begin(), rescope_offsets.end(), idx);
   } else {
      it_scope = std::upper_bound(rescope_offsets.begin(), rescope_offsets.end(), idx);
   }
   // Go back one index as we have to reference the previous scope.
   return std::distance(rescope_offsets.begin(), it_scope) - 1;
}

Suboperator& Pipeline::attachSuboperator(SuboperatorArc subop) {
   if (!subop->isSource()) {
      // Update the pipeline graph.
      auto scope = rescope_offsets.size() - 1;
      for (const auto& depends : subop->getSourceIUs()) {
         // Resolve input.
         if (auto provider = tryGetProvider(scope, *depends)) {
            // Add edges.
            graph.outgoing_edges[provider].push_back(subop.get());
            graph.incoming_edges[subop.get()].push_back(provider);
         }
      }
   }
   // Get the scoping behaviour of the suboperator.
   const auto [behaviour, new_sel] = subop->scopingBehaviour();
   if (behaviour == Suboperator::ScopingBehaviour::RescopeRetain) {
      // Install a new scope which rescopes the source IUs on the suboperator.
      auto new_scope = IUScope::retain(new_sel, *scopes.back(), *subop, subop->getSourceIUs());
      scopes.push_back(std::make_shared<IUScope>(std::move(new_scope)));
      rescope_offsets.push_back(suboperators.size());
   } else {
      if (behaviour == Suboperator::ScopingBehaviour::RescopeRewire) {
         // Install a new scope which does not contain any IUs for now.
         auto new_scope = IUScope::rewire(new_sel, *scopes.back());
         scopes.push_back(std::make_shared<IUScope>(std::move(new_scope)));
         rescope_offsets.push_back(suboperators.size());
      }
      // Register newly produced IUs within the current scope.
      for (auto iu : subop->getIUs()) {
         // Add the produced IU to the (potentially) new scope.
         scopes.back()->registerIU(*iu, *subop);
      }
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