#include "algebra/suboperators/HtLookupSubop.h"
#include "algebra/CompilationContext.h"
#include "codegen/IRBuilder.h"
#include "runtime/Runtime.h"

#include <memory>

namespace inkfuse {

const char* HtLookupSubopState::name = "HtLookupSubopState";

void HtLookupSubop::registerRuntime() {
   RuntimeStructBuilder{HtLookupSubopState::name}
      .addMember("hash_table", IR::Pointer::build(IR::Void::build()));
}

std::unique_ptr<HtLookupSubop> HtLookupSubop::build(const RelAlgOp* source, const IU& hashes_, HashTable* hash_table_) {
   return std::unique_ptr<HtLookupSubop>(new HtLookupSubop(source, hashes_, hash_table_));
}

HtLookupSubop::HtLookupSubop(const RelAlgOp* source, const IU& hashes_, HashTable* hash_table_)
   : TemplatedSuboperator<HtLookupSubopState>(source, {&produced}, {&hashes_}), hash_table(hash_table_), hashes(hashes_), produced(IR::Pointer::build(IR::Void::build())) {
}

void HtLookupSubop::consumeAllChildren(CompilationContext& context) {
   auto& builder = context.getFctBuilder();
   const auto& program = context.getProgram();
   const auto& in = context.getIUDeclaration(hashes);

   // Get the hash table pointer from the backing global state.
   auto gstate_raw_ptr = context.accessGlobalState(*this);
   auto gstate_casted = IR::CastExpr::build(std::move(gstate_raw_ptr), IR::Pointer::build(program.getStruct(HtLookupSubopState::name)));
   auto hash_table_ptr = IR::StructAccessExpr::build(std::move(gstate_casted), "hash_table");

   // Perform the lookup.
   auto ht_lookup = context.getRuntimeFunction("ht_lookup").get();
   std::vector<IR::ExprPtr> args;
   // First argument is the hash table, second argument is the hash.
   args.push_back(std::move(hash_table_ptr));
   args.push_back(IR::VarRefExpr::build(in));

   // Declare the output.
   auto iu_name = context.buildIUIdentifier(produced);
   auto declare = IR::DeclareStmt::build(iu_name, produced.type);
   context.declareIU(produced, *declare);

   // And build the assignment into the output IU.
   IR::ExprPtr lookup_out =
      IR::InvokeFctExpr::build(*ht_lookup, std::move(args));
   auto assign =
      IR::AssignmentStmt::build(
         *declare, std::move(lookup_out));
   builder.appendStmt(std::move(declare));
   builder.appendStmt(std::move(assign));

   // And notify consumer that IUs are ready.
   context.notifyIUsReady(*this);
}

void HtLookupSubop::setUpStateImpl(const ExecutionContext& context) {
   state->hash_table = static_cast<void*>(hash_table);
}

std::string HtLookupSubop::id() const {
   return "HtLookupSubop";
}

}