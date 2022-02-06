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
   // Build the root scope of this compilation context.
   scopes.push_back(std::make_unique<Scope>(0));
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
   auto it_scope = std::lower_bound(rescope_offsets.cbegin(), rescope_offsets.cend(), idx);
   // Go back one index as we have to reference the previous scope.
   return std::distance(rescope_offsets.begin(), it_scope) - 1;
}

void Pipeline::attachSuboperator(SuboperatorPtr subop) {

}



}