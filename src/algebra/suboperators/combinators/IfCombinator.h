#ifndef INKFUSE_IFCOMBINATOR_H
#define INKFUSE_IFCOMBINATOR_H

#include "algebra/suboperators/Suboperator.h"
#include <sstream>

namespace inkfuse {

/// The IfCombinator provides a suboperator combinator
/// which evaluates the suboperator only on rows where a filtered
/// IU is set (IU can be arbitrary type - doing C-style cast to bool).
/// This is for example used to only advance hash-table iterators on rows
/// that have not yet found a suitable match.
/// The combinators are somewhat dynamic because they go over a set of IUs
/// to produce a condition. The important part here is that all possible
/// combinators are known by InkFuse at system compile time. This makes it
/// possible to not go through abstract runtime state.
template <class Wrapped>
struct IfCombinator : Wrapped {
   template <typename... Args>
   static SuboperatorArc build(std::vector<const IU*> condition_, Args&&... args);

   void consumeAllChildren(CompilationContext& context) override;

   void consume(const IU& iu, CompilationContext& context) override;

   std::string id() const override;

   private:
   template <typename... Args>
   IfCombinator(std::vector<const IU*> condition_, Args&&... args);

   /// Input IUs which all have to evaluate to true for the nexted suboperator code to be called.
   std::vector<const IU*> condition;
};

template <class Wrapped>
template <typename... Args>
SuboperatorArc IfCombinator<Wrapped>::build(std::vector<const IU*> condition_, Args&&... args) {
   return SuboperatorArc{new IfCombinator<Wrapped>(std::move(condition_), std::forward(args)...)};
}

template <class Wrapped>
void IfCombinator<Wrapped>::consumeAllChildren(CompilationContext& context) {
   if constexpr (Wrapped::hasConsumeAll) {
      Wrapped::consumeAllChildren(context);
   }
}

template <class Wrapped>
void IfCombinator<Wrapped>::consume(const IU& iu, CompilationContext& context) {
   if constexpr (Wrapped::hasConsume) {
      Wrapped::consume(iu, context);
   }
}

template <class Wrapped>
std::string IfCombinator<Wrapped>::id() const {
   std::stringstream str;
   str << "IfCombinator_";
   for (auto& type : condition) {
      str << type->type->id() << "_";
   }
   str << Wrapped::id();
   return str.str();
}

template <class Wrapped>
template <typename... Args>
IfCombinator<Wrapped>::IfCombinator(std::vector<const IU*> condition_, Args&&... args)
   : Wrapped(std::forward(args)...), condition(std::move(condition_)) {
}

}

#endif //INKFUSE_IFCOMBINATOR_H
