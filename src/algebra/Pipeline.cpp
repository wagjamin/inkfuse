#include "algebra/Pipeline.h"
#include "algebra/suboperators/Suboperator.h"
#include <algorithm>

namespace inkfuse {


void Scope::registerProducer(const IU& iu, Suboperator &op)
{
   iu_producers[&iu] = &op;
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
   IURef ref(*selection);
   return getColumn(ref);
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

Suboperator & Pipeline::getProvider(IUScoped iu)
{
   return scopes[iu.scope_id]->getProducer(iu.iu);
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

void Pipeline::attachSuboperator(SuboperatorPtr subop) {
   if (subop->isSink()) {
      sinks.emplace(suboperators.size());
   }
   // Rescope the pipeline.
   subop->rescopePipeline(*this);
   // Register IUs within the current scope.
   for (auto iu: subop->getIUs()) {
      scopes.back()->registerProducer(*iu, *subop);
   }
   suboperators.push_back(std::move(subop));
}

}