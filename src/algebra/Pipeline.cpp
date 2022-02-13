#include "algebra/Pipeline.h"
#include "algebra/suboperators/Suboperator.h"
#include "algebra/suboperators/sinks/FuseChunkSink.h"
#include "algebra/suboperators/sources/FuseChunkSource.h"
#include <algorithm>

namespace inkfuse {

void Scope::registerProducer(const IU& iu, Suboperator& op) {
   iu_producers[&iu] = &op;
}

Suboperator& Scope::getProducer(const IU& iu) const {
   return *iu_producers.at(&iu);
}

Pipeline::Pipeline() {
}

std::unique_ptr<Pipeline> Pipeline::repipe(size_t start, size_t end, bool materialize_all) const {
   auto new_pipe = std::make_unique<Pipeline>();

   // Resolve in_scope of the first operator.
   auto in_scope = resolveOperatorScope(*suboperators[start]);
   auto out_scope = resolveOperatorScope(*suboperators[end - 1], false);
   size_t in_scope_end = in_scope + 1 == scopes.size() ? suboperators.size() : rescope_offsets[in_scope + 1];
   size_t out_scope_end = out_scope + 1 == scopes.size() ? suboperators.size() : rescope_offsets[out_scope + 1];

   // Find out which IUs we will need in the first in_scope that are not in_provided.
   std::set<const IU*> in_provided;
   std::set<const IU*> in_required;
   std::for_each(
      suboperators.begin() + start,
      suboperators.begin() + in_scope_end,
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

   // Build a FuseChunkSource for those.
   // TODO

   // Copy all intermediate operators.
   std::for_each(
      suboperators.begin() + start,
      suboperators.begin() + end,
      [&](const SuboperatorArc& op) {
         new_pipe->attachSuboperator(op);
      });

   // All intermediate scopes can be copied as-is.
   for (size_t copy_scope = in_scope + 1; copy_scope < out_scope; ++copy_scope) {
      // TODO?!
   }

   // Find output IUs which have to be materialized.
   std::set<const IU*> out_required;
   if (materialize_all) {
      // Find all produced IUs which are not loop drivers.
      std::for_each(
         suboperators.begin() + rescope_offsets[out_scope],
         suboperators.begin() + end,
         [&](const SuboperatorArc& subop) {
            for (auto iu : subop->getIUs()) {
               if (!subop->isSource()) {
                  out_required.insert(iu);
               }
            }
         });
   } else {
      // Find out which IUs we will need after this interval.
      std::for_each(
         suboperators.begin() + end,
         suboperators.begin() + out_scope_end,
         [&](const SuboperatorArc& subop) {
            for (auto iu : subop->getSourceIUs()) {
               if (!subop->isSource()) {
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

const Scope& Pipeline::getScope(size_t id) const {
   assert(id < scopes.size());
   return *scopes[id];
}

void Pipeline::rescope(ScopePtr new_scope) {
   scopes.push_back(std::move(new_scope));
   rescope_offsets.push_back(scopes.size() - 1);
}

const std::vector<Suboperator*>& Pipeline::getConsumers(Suboperator& subop) const {
   return graph.outgoing_edges.at(&subop);
}

const std::vector<Suboperator*>& Pipeline::getProducers(Suboperator& subop) const {
   return graph.incoming_edges.at(&subop);
}

Suboperator& Pipeline::getProvider(IUScoped iu) const {
   return scopes.at(iu.scope_id)->getProducer(iu.iu);
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
      auto scope = rescope_offsets.size() - 1;
      for (const auto& depends : subop->getSourceIUs()) {
         // Resolve input.
         IUScoped scoped_iu{*depends, scope};
         auto& provider = getProvider(scoped_iu);
         // Add edges.
         graph.outgoing_edges[&provider].push_back(subop.get());
         graph.incoming_edges[subop.get()].push_back(&provider);
      }
   }
   // Rescope the pipeline (if necessary).
   subop->rescopePipeline(*this);
   // Register new IUs within the current scope.
   for (auto iu : subop->getIUs()) {
      // Add the produced IU to the (potentially) new scope.
      scopes.back()->registerProducer(*iu, *subop);
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