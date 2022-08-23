#include "algebra/suboperators/RuntimeFunctionSubop.h"
#include "algebra/CompilationContext.h"
#include "codegen/IRBuilder.h"
#include "runtime/Runtime.h"

#include <memory>

namespace inkfuse {

const char* RuntimeFunctionSubopState::name = "RuntimeFunctionSubopState";

void RuntimeFunctionSubop::registerRuntime() {
   RuntimeStructBuilder{RuntimeFunctionSubopState::name}
      .addMember("this_object", IR::Pointer::build(IR::Void::build()));
}

std::unique_ptr<RuntimeFunctionSubop> RuntimeFunctionSubop::htLookup(const RelAlgOp* source, const IU& hashes, void* hash_table) {
   return std::unique_ptr<RuntimeFunctionSubop>(new RuntimeFunctionSubop(source, "ht_sk_lookup", hashes, hash_table));
}

RuntimeFunctionSubop::RuntimeFunctionSubop(const RelAlgOp* source, std::string fct_name_, const IU& arg_, void* this_object_)
   : TemplatedSuboperator<RuntimeFunctionSubopState>(source, {&produced}, {&arg_}), fct_name(std::move(fct_name_)), this_object(this_object_), arg(arg_), produced(IR::Pointer::build(IR::Void::build())) {
}

void RuntimeFunctionSubop::consumeAllChildren(CompilationContext& context) {
   auto& builder = context.getFctBuilder();
   const auto& program = context.getProgram();
   const auto& in = context.getIUDeclaration(arg);

   // Get the hash table pointer from the backing global state.
   auto gstate_raw_ptr = context.accessGlobalState(*this);
   auto gstate_casted = IR::CastExpr::build(std::move(gstate_raw_ptr), IR::Pointer::build(program.getStruct(RuntimeFunctionSubopState::name)));
   auto hash_table_ptr = IR::StructAccessExpr::build(std::move(gstate_casted), "this_object");

   // Call the function.
   auto ht_lookup = context.getRuntimeFunction(fct_name).get();
   std::vector<IR::ExprPtr> args;
   // First argument is the runtime object, second argument is the provided IU.
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

void RuntimeFunctionSubop::setUpStateImpl(const ExecutionContext& context) {
   state->this_object = static_cast<void*>(this_object);
}

std::string RuntimeFunctionSubop::id() const {
   return "RuntimeFunctionSubop";
}

}