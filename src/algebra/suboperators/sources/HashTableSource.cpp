#include "algebra/suboperators/sources/HashTableSource.h"
#include "exec/FuseChunk.h"
#include "runtime/Runtime.h"

namespace inkfuse {

const char* HashTableSourceState::name = "HashTableSourceState";

void HashTableSourceState::registerRuntime() {
   RuntimeStructBuilder{HashTableSourceState::name}
      .addMember("hash_table", IR::Pointer::build(IR::Void::build()))
      .addMember("it_ptr_start", IR::Pointer::build(IR::Char::build()))
      .addMember("it_idx_start", IR::UnsignedInt::build(8))
      .addMember("it_ptr_end", IR::Pointer::build(IR::Char::build()))
      .addMember("it_idx_end", IR::UnsignedInt::build(8));
}

template <class HashTable>
HashTableSource<HashTable>::HashTableSource(const RelAlgOp* source, const IU& produced_iu, DefferredStateInitializer* deferred_state_)
   : TemplatedSuboperator<HashTableSourceState>(source, {&produced_iu}, {}), deferred_state(deferred_state_) {
}

template <class HashTable>
std::string HashTableSource<HashTable>::id() const {
   std::string id = "hash_table_source_";
   return id + HashTable::ID;
}

template <class HashTable>
SuboperatorArc HashTableSource<HashTable>::build(const RelAlgOp* source, const IU& produced_iu, DefferredStateInitializer* deferred_state_) {
   return std::unique_ptr<HashTableSource>{new HashTableSource(source, produced_iu, deferred_state_)};
}

template <class HashTable>
Suboperator::PickMorselResult HashTableSource<HashTable>::pickMorsel(size_t thread_id) {
   void* ht_ptr = deferred_state->access(thread_id);
   assert(ht_ptr);
   HashTable* hash_table = reinterpret_cast<HashTable*>(ht_ptr);
   HashTableSourceState& state = (*states)[thread_id];
   if (state.it_ptr_end == nullptr) {
      // The last morsel went until the hash table end. We are done.
      return Suboperator::NoMoreMorsels{};
   }
   // The current end becomes the new start.
   state.it_ptr_start = state.it_ptr_end;
   state.it_idx_start = state.it_idx_end;
   // Advance the end iterator by one morsel.
   size_t entries_found = 0;
   while (entries_found < DEFAULT_CHUNK_SIZE && (state.it_ptr_end != nullptr)) {
      hash_table->iteratorAdvance(&state.it_ptr_end, &state.it_idx_end);
      entries_found++;
   }
   return PickedMorsel{
      .morsel_size = entries_found,
      .pipeline_progress = static_cast<double>(state.it_idx_end) / hash_table->capacity(),
   };
}

template <class HashTable>
void HashTableSource<HashTable>::open(CompilationContext& context) {
   auto& builder = context.getFctBuilder();
   const auto& program = context.getProgram();
   const auto& iu = *provided_ius.front();

   IR::Stmt* iu_decl;
   IR::Stmt* decl_it_end_ptr;
   {
      // Within the preamble, extract the initial iterator state of the morsel.
      // We cannot work on the morsel directly, as otherwise a next vectorized primitive on the
      // same morsel might see the iterator having moved too far.
      std::deque<IR::StmtPtr> preamble_stmts;
      auto p_append = [&](IR::StmtPtr ptr) { return preamble_stmts.emplace_back(std::move(ptr)).get(); };

      // Build variables for global state. Hash table iterator declaration also serves as produced IU.
      auto var_name = this->getVarIdentifier().str();
      auto iu_name = context.buildIUIdentifier(iu);
      iu_decl = p_append(IR::DeclareStmt::build(std::move(iu_name), IR::Pointer::build(IR::Char::build())));
      decl_it_end_ptr = p_append(IR::DeclareStmt::build(var_name + "_it_ptr_end", IR::Pointer::build(IR::Char::build())));
      decl_it_idx = p_append(IR::DeclareStmt::build(var_name + "_it_idx", IR::UnsignedInt::build(8)));
      decl_ht = p_append(IR::DeclareStmt::build(var_name + "_ht", IR::Pointer::build(IR::Void::build())));
      context.declareIU(iu, *iu_decl);
      // Copy values from the global state into local variables.
      auto gstate_extract_into = [&](std::string member, IR::Stmt& stmt) {
         auto state_expr = context.accessGlobalState(*this);
         auto state_cast = IR::CastExpr::build(std::move(state_expr), IR::Pointer::build(program.getStruct(HashTableSourceState::name)));
         auto access_expr = IR::StructAccessExpr::build(std::move(state_cast), std::move(member));
         preamble_stmts.push_back(IR::AssignmentStmt::build(stmt, std::move(access_expr)));
      };
      gstate_extract_into("it_ptr_start", *iu_decl);
      gstate_extract_into("it_ptr_end", *decl_it_end_ptr);
      gstate_extract_into("it_idx_start", *decl_it_idx);
      gstate_extract_into("hash_table", *decl_ht);
      builder.getRootBlock().appendStmts(std::move(preamble_stmts));
   }

   // Iterate until we have reached the end of the morsel.
   // Next up we create the driving for-loop.
   this->opt_while = builder.buildWhile(
      IR::ArithmeticExpr::build(
         IR::VarRefExpr::build(*iu_decl),
         IR::VarRefExpr::build(*decl_it_end_ptr),
         IR::ArithmeticExpr::Opcode::Neq));
   {
      // Generate code for downstream consumers.
      context.notifyIUsReady(*this);
   }
}

template <class HashTable>
void HashTableSource<HashTable>::close(CompilationContext& context) {
   auto& builder = context.getFctBuilder();
   const auto& iu = *provided_ius.front();
   const auto& iu_decl = context.getIUDeclaration(iu);
   // Advance the hash table iterator.
   auto runtime_fct = context.getRuntimeFunction("ht_" + HashTable::ID + "_it_advance").get();
   std::vector<IR::ExprPtr> args_exprs;
   args_exprs.reserve(3);
   // Hash table gets passed as-is.
   args_exprs.push_back(IR::VarRefExpr::build(*decl_ht));
   // Other arguments get referenced, as they actually get updated.
   args_exprs.push_back(IR::RefExpr::build(IR::VarRefExpr::build(iu_decl)));
   args_exprs.push_back(IR::RefExpr::build(IR::VarRefExpr::build(*decl_it_idx)));
   IR::ExprPtr invoke_expr =
      IR::InvokeFctExpr::build(*runtime_fct, std::move(args_exprs));
   auto invoke_stmt = IR::InvokeFctStmt::build(std::move(invoke_expr));
   builder.appendStmt(std::move(invoke_stmt));
   // And close the loop.
   opt_while->End();
   opt_while.reset();
}

template <class HashTable>
void HashTableSource<HashTable>::setUpStateImpl(const ExecutionContext& context) {
   assert(deferred_state);

   for (size_t k = 0; k < context.getNumThreads(); ++k) {
      auto& state = (*states)[k];
      void* ht_ptr = deferred_state->access(k);
      state.hash_table = ht_ptr;
      assert(ht_ptr);
      HashTable* hash_table = reinterpret_cast<HashTable*>(ht_ptr);
      // Initialize start to point to the first slot of the hash table.
      hash_table->iteratorStart(&(state.it_ptr_start), &(state.it_idx_start));
      // Set the end iterator to the same value. This way the first pickMorsel will work.
      state.it_ptr_end = state.it_ptr_start;
      state.it_idx_end = state.it_idx_start;
   }
}

// Explicitly instantiate templates.
template class HashTableSource<HashTableSimpleKey>;
template class HashTableSource<HashTableComplexKey>;
template class HashTableSource<HashTableDirectLookup>;

}
