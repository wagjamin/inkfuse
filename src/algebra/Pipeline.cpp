#include "algebra/Pipeline.h"
#include "algebra/suboperators/Suboperator.h"
#include <algorithm>

namespace inkfuse {


void Scope::registerProducer(const IU& iu, Suboperator &op)
{
   iu_producers[&iu] = &op;
   // TODO cleaner to not build the chunk in the actual pipeline, but rather down the line.
   chunk->attachColumn(iu);
}

Suboperator& Scope::getProducer(const IU& iu) const
{
   return *iu_producers.at(&iu);
}

Column &Scope::getColumn(const IU& iu) const
{
   return chunk->getColumn(iu);
}

Column &Scope::getSel() const
{
   return getColumn(*selection);
}

Pipeline::Pipeline()
{
}

Scope & Pipeline::getCurrentScope() {
   assert(!scopes.empty());
   return **scopes.end();
}

void Pipeline::rescope(ScopePtr new_scope)
{
   scopes.push_back(std::move(new_scope));
   rescope_offsets.push_back(scopes.size() - 1);
}

Column &Pipeline::getScopedIU(IUScoped iu)
{
   assert(iu.scope_id < scopes.size());
   return scopes[iu.scope_id]->getColumn(iu.iu);
}


Column &Pipeline::getSelectionCol(size_t scope_id)
{
   assert(scope_id < scopes.size());
   return scopes[scope_id]->getSel();
}

const std::vector<Suboperator*>& Pipeline::getConsumers(Suboperator& subop) const
{
   return graph.outgoing_edges.at(&subop);
}

const std::vector<Suboperator*>& Pipeline::getProducers(Suboperator& subop) const
{
   return graph.incoming_edges.at(&subop);
}

Suboperator & Pipeline::getProvider(IUScoped iu) const
{
   return scopes.at(iu.scope_id)->getProducer(iu.iu);
}

size_t Pipeline::resolveOperatorScope(const Suboperator& op) const
{
   // Find the index of the suboperator in the topological sort.
   auto it = std::find_if(suboperators.cbegin(), suboperators.cend(), [&op](const SuboperatorPtr& target){
     return target.get() == &op;
   });
   auto idx = std::distance(suboperators.cbegin(), it);

   // Find the index of the scope of the input ius of the target operator.
   if (idx == 0) {
      // Source is a special operator which also resolves to scope 0.
      // This is fine since the source will never have input ius it needs to resolve.
      return 0;
   }
   auto it_scope = std::lower_bound(rescope_offsets.begin(), rescope_offsets.end(), idx);
   // Go back one index as we have to reference the previous scope.
   return std::distance(rescope_offsets.begin(), it_scope) - 1;
}

Suboperator& Pipeline::attachSuboperator(SuboperatorPtr subop) {
   if (subop->isSink()) {
      sinks.emplace(suboperators.size());
   }
   if (!subop->isSource()) {
      auto scope = rescope_offsets.size() - 1;
      for (const auto& depends: subop->getSourceIUs()) {
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
   for (auto iu: subop->getIUs()) {
      scopes.back()->registerProducer(*iu, *subop);
   }
   suboperators.push_back(std::move(subop));
   return *suboperators.back();
}

const std::vector<SuboperatorPtr>& Pipeline::getSubops() const
{
   return suboperators;
}

Pipeline& PipelineDAG::getCurrentPipeline() const
{
   assert(!pipelines.empty());
   return *pipelines.back();
}

Pipeline& PipelineDAG::buildNewPipeline()
{
   pipelines.push_back(std::make_unique<Pipeline>());
   return *pipelines.back();
}

const std::vector<PipelinePtr>& PipelineDAG::getPipelines() const
{
   return pipelines;
}

}