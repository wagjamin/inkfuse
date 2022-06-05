#ifndef INKFUSE_IUSCOPE_H
#define INKFUSE_IUSCOPE_H

#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

namespace inkfuse {

struct IU;
struct Suboperator;

/// An IUScope is at the heart of pipelines within inkfuse. An IU describes some data (usually a column)
/// as it passes through a pipeline.
/// In a simplest example, an IU might be created from a table scan, get fed into a filter and then be
/// used while probing a hash table.
/// Now, a scope represents the maximum number of suboperators during whose execution
/// a selection vector remains valid. In other words, all suboperators which do not necessarily represent a
/// 1:1 mapping between an input and and output tuple generate a new scope.
/// Note that this corresponds very closely to actual scopes in the generated code.
struct IUScope {
   public:
   IUScope(const IU* filter_iu_): filter_iu(filter_iu_) {};
   virtual ~IUScope() = default;

   /// Create a new scope which keeps some previous IU identifiers alive.
   /// However, it installs a new filter IU on top of them. This filter IU is part of the reatined sets.
   static IUScope retain(const IUScope& parent, const IU* filter_iu, const std::set<const IU*>& retained_ius);
   /// Create a new scope that drops all IU identifiers.
   /// This will drop all IUs which will have to be redeclared on a new producer.
   static IUScope rewire(const IU* filter_iu);

   /// Register a new IU in this scope.
   void registerIU(const IU& iu, Suboperator& op);

   /// Does an IU exist within this scope?
   bool exists(const IU& iu) const;

   /// Get the id of a given IU within this scope. These are needed to create unique
   /// variable identifiers in the generated code.
   uint64_t getId(const IU& iu) const;

   /// Get the producing sub-operator of a given IU.
   Suboperator& getProducer(const IU& iu) const;

   /// The the filter IU of this scope.
   const IU* getFilterIU() const;

   /// Description of a IU which can be uniquely identified in this scope.
   struct IUDescription {
      const IU* iu;
      size_t id;
   };

   /// Get all the IUs stored within this chunk.
   std::vector<IUDescription> getIUs() const;

   protected:
   /// Register an IU with a given ID.
   void registerIU(const IU& iu, Suboperator& op, uint64_t id);
   /// Generate a new global IU identifier that is not in use yet.
   static uint64_t generateId();

   /// A mapping from IUs to the backing IU identifiers.
   std::unordered_map<const IU*, uint64_t> iu_mapping;
   /// A mapping from the IUs to the backing IU producers.
   std::unordered_map<const IU*, Suboperator*> iu_producers;
   /// The backing filter IU in this scope. Is part of the above mappings.
   /// If the filter_iu is null, the current scope is not filtered.
   const IU* filter_iu;
};

using IUScopeArc = std::shared_ptr<IUScope>;

}

#endif //INKFUSE_IUSCOPE_H
