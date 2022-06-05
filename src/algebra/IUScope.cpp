#include "algebra/IUScope.h"
#include <atomic>
#include <cassert>

namespace inkfuse {

IUScope IUScope::retain(const IU* filter_iu, const IUScope& parent, Suboperator& producer, const std::unordered_set<const IU*>& retained_ius) {
   IUScope new_scope(filter_iu, parent.scope_id + 1);
   // Iterate over the retained IUs and register them locally under the old id.
   for (auto retained : retained_ius) {
      const auto id = parent.getScopeId(*retained);
      new_scope.registerIU(*retained, producer, id);
   }
   return new_scope;
}

IUScope IUScope::rewire(const IU* filter_iu, const IUScope& parent) {
   IUScope new_scope(filter_iu, parent.scope_id + 1);
   return new_scope;
}

void IUScope::registerIU(const IU& iu, Suboperator& op) {
   registerIU(iu, op, scope_id);
}

bool IUScope::exists(const IU& iu) const {
   return iu_mapping.count(&iu) != 0;
}

size_t IUScope::getScopeId(const IU& iu) const {
   return iu_mapping.at(&iu);
}

size_t IUScope::getIdThisScope() const {
   return scope_id;
}

Suboperator& IUScope::getProducer(const IU& iu) const {
   return *iu_producers.at(&iu);
}

const IU* IUScope::getFilterIU() const {
   return filter_iu;
}

std::vector<IUScope::IUDescription> IUScope::getIUs() const {
   std::vector<IUDescription> result;
   result.reserve(iu_mapping.size());
   for (const auto& [iu, source_scope] : iu_mapping) {
      result.push_back({.iu = iu, .source_scope=source_scope});
   }
   return result;
}

void IUScope::registerIU(const IU& iu, Suboperator& op, uint64_t id) {
   assert(!iu_mapping.count(&iu));
   iu_mapping[&iu] = id;
   iu_producers[&iu] = &op;
}

}