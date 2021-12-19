// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_INFRA_TEMPLATE_H_
#define INCLUDE_IMLAB_INFRA_TEMPLATE_H_
//---------------------------------------------------------------------------
#include <numeric>
#include <tuple>
//---------------------------------------------------------------------------
// Apply function f on the tuple elements with an index sequence
template <typename F, typename... Types, std::size_t... Indexes>
void ForEachInTuple(const std::tuple<Types...>& tuple, F func, std::index_sequence<Indexes...>) {
   using expander = int[];
   (void) expander{0, ((void) func(std::get<Indexes>(tuple)), 0)...};
}
//---------------------------------------------------------------------------
// Apply function f on the tuple elements
template <typename F, typename... Types>
void ForEachInTuple(const std::tuple<Types...>& tuple, F func) {
   ForEachInTuple(tuple, func, std::make_index_sequence<sizeof...(Types)>());
}
//---------------------------------------------------------------------------
// Pairwise compare two tuples with an index sequence
template <typename... Types, std::size_t... Indexes>
bool PairwiseCompare(const std::tuple<Types...>& left, const std::tuple<Types...>& right,
                     std::index_sequence<Indexes...>) {
   std::array<bool, std::index_sequence<Indexes...>::size()> results{
      (std::get<Indexes>(left) == std::get<Indexes>(right))...};
   auto conjunction = [](bool l, bool r) { return l & r; };
   return std::accumulate(results.begin(), results.end(), true, conjunction);
}
//---------------------------------------------------------------------------
// Pairwise compare two tuples
template <typename... Types>
bool PairwiseCompare(const std::tuple<Types...>& left, const std::tuple<Types...>& right) {
   return PairwiseCompare(left, right, std::make_index_sequence<sizeof...(Types)>());
}
//---------------------------------------------------------------------------
#endif // INCLUDE_IMLAB_INFRA_TEMPLATE_H_
//---------------------------------------------------------------------------
