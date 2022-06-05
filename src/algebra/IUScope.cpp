#include "algebra/IUScope.h"
#include <atomic>
#include <cassert>

namespace inkfuse {

namespace {
// Atomicity is not required as long as inkfuse is single threaded, but
// let's still increase the guarantees we can provide here.
std::atomic<uint64_t> iu_identifier = 0;
}

uint64_t IUScope::generateId()
{
   return iu_identifier.fetch_add(1);
}

IUScope IUScope::retain(const IUScope& parent, const IU* filter_iu, const std::set<const IU*>& retained_ius)
{
   IUScope new_scope(filter_iu);
   // Iterate over the retained IUs and register them locally under the old id.
   for (auto retained: retained_ius) {
      const auto id = parent.getId(*retained);
      auto& producer = parent.getProducer(*retained);
      new_scope.registerIU(*retained, producer, id);
   }
   return new_scope;
}

IUScope IUScope::rewire(const IU* filter_iu) {
   IUScope new_scope(filter_iu);
   return new_scope;
}

void IUScope::registerIU(const IU& iu, Suboperator& op) {
   const auto new_id = generateId();
   registerIU(iu, op, new_id);
}

bool IUScope::exists(const IU& iu) const
{
   return iu_mapping.count(&iu) != 0;
}

uint64_t IUScope::getId(const IU& iu) const {
   return iu_mapping.at(&iu);
}

Suboperator& IUScope::getProducer(const IU& iu) const {
   return *iu_producers.at(&iu);
}

const IU* IUScope::getFilterIU() const {
   return filter_iu;
}

std::vector<IUScope::IUDescription> IUScope::getIUs() const
{
   std::vector<IUDescription> result;
   result.reserve(iu_mapping.size());
   for (const auto& [iu, id]: iu_mapping) {
      result.push_back({.iu = iu, .id = id});
   }
   return result;
}

void IUScope::registerIU(const IU& iu, Suboperator& op, uint64_t id)
{
   assert(!iu_mapping.count(&iu));
   iu_mapping[&iu] = id;
   iu_producers[&iu] = &op;
}

}