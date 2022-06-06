#ifndef INKFUSE_IUSCOPE_H
#define INKFUSE_IUSCOPE_H

#include <memory>
#include <unordered_set>
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
   IUScope(const IU* filter_iu_, size_t scope_id_): scope_id(scope_id_), filter_iu(filter_iu_) {};
   virtual ~IUScope() = default;

   /// Create a new scope which keeps some previous IU identifiers alive, but under a new producer.
   /// This also installs a new filter IU on top of them. This filter IU is part of the reatined sets.
   static IUScope retain(const IU* filter_iu, const IUScope& parent, Suboperator& producer, const std::vector<const IU*>& retained_ius);
   /// Create a new scope that drops all IU identifiers.
   /// This will drop all IUs which will have to be redeclared on a new producer.
   static IUScope rewire(const IU* filter_iu, const IUScope& parent);

   /// Register a new IU in this scope.
   void registerIU(const IU& iu, Suboperator& op);

   /// Does an IU exist within this scope?
   bool exists(const IU& iu) const;

   /// Get the scope id of a given IU. This represents the source scope in which
   /// this IU was created.
   size_t getScopeId(const IU& iu) const;

   /// Get the scope id of this scope. This will be returned by getScopeId of newly registered IUs.
   size_t getIdThisScope() const;

   /// Get the producing sub-operator of a given IU.
   Suboperator& getProducer(const IU& iu) const;

   /// The the filter IU of this scope.
   const IU* getFilterIU() const;

   /// Description of a IU which can be uniquely identified in this scope.
   struct IUDescription {
      /// Which IU are we referencing?
      const IU* iu;
      /// What is the id of the source scope that created the scoped IU?
      size_t source_scope;
   };

   /// Get all the IUs stored within this chunk.
   std::vector<IUDescription> getIUs() const;

   protected:
   /// Register an IU with a given ID.
   void registerIU(const IU& iu, Suboperator& op, size_t root_scope);

   /// A mapping from IUs to the backing scope identifiers (the index of the root scope).
   std::unordered_map<const IU*, size_t> iu_mapping;
   /// A mapping from the IUs to the backing IU producers.
   std::unordered_map<const IU*, Suboperator*> iu_producers;
   /// The unique scope id within the backing pipeline.
   size_t scope_id;
   /// The backing filter IU in this scope. Is part of the above mappings.
   /// If the filter_iu is null, the current scope is not filtered.
   const IU* filter_iu;
};

using IUScopeArc = std::shared_ptr<IUScope>;

}

#endif //INKFUSE_IUSCOPE_H
