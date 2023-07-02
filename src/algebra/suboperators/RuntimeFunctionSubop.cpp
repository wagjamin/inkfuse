#include "algebra/suboperators/RuntimeFunctionSubop.h"
#include "algebra/CompilationContext.h"
#include "codegen/IRBuilder.h"
#include "runtime/Runtime.h"
#include <memory>
#include <sstream>

namespace inkfuse {

const char* RuntimeFunctionSubopState::name = "RuntimeFunctionSubopState";

void RuntimeFunctionSubop::registerRuntime() {
   RuntimeStructBuilder{RuntimeFunctionSubopState::name}
      .addMember("this_object", IR::Pointer::build(IR::Void::build()));
}

std::unique_ptr<RuntimeFunctionSubop> RuntimeFunctionSubop::htInsert(const inkfuse::RelAlgOp* source, const inkfuse::IU* pointers_, const inkfuse::IU& key_, std::vector<const IU*> pseudo_ius_, DefferredStateInitializer* state_init_) {
   std::string fct_name = "ht_sk_insert";
   std::vector<const IU*> in_ius{&key_};
   for (auto pseudo : pseudo_ius_) {
      // Pseudo IUs are used as input IUs in the backing graph, but do not influence arguments.
      in_ius.push_back(pseudo);
   }
   std::vector<bool> ref{key_.type->id() != "ByteArray" && key_.type->id() != "Ptr_Char"};
   std::vector<const IU*> out_ius_;
   if (pointers_) {
      out_ius_.push_back(pointers_);
   }
   std::vector<const IU*> args{&key_};
   return std::unique_ptr<RuntimeFunctionSubop>(
      new RuntimeFunctionSubop(
         source,
         state_init_,
         std::move(fct_name),
         std::move(in_ius),
         std::move(out_ius_),
         std::move(args),
         std::move(ref),
         pointers_));
}

std::unique_ptr<RuntimeFunctionSubop> RuntimeFunctionSubop::htLookupDisable(const RelAlgOp* source, const IU& pointers_, const IU& keys_, std::vector<const IU*> pseudo_ius_, DefferredStateInitializer* state_init_) {
   std::string fct_name = "ht_sk_lookup_disable";
   std::vector<const IU*> in_ius{&keys_};
   for (auto pseudo : pseudo_ius_) {
      // Pseudo IUs are used as input IUs in the backing graph, but do not influence arguments.
      in_ius.push_back(pseudo);
   }
   std::vector<bool> ref{keys_.type->id() != "ByteArray" && keys_.type->id() != "Ptr_Char"};
   std::vector<const IU*> out_ius_{&pointers_};
   std::vector<const IU*> args{&keys_};
   const IU* out = &pointers_;
   return std::unique_ptr<RuntimeFunctionSubop>(
      new RuntimeFunctionSubop(
         source,
         state_init_,
         std::move(fct_name),
         std::move(in_ius),
         std::move(out_ius_),
         std::move(args),
         std::move(ref),
         out));
}

std::unique_ptr<RuntimeFunctionSubop> RuntimeFunctionSubop::htNoKeyLookup(const RelAlgOp* source, const IU& pointers_, const IU& input_dependency, DefferredStateInitializer* state_init_) {
   std::string fct_name = "ht_nk_lookup";
   std::vector<const IU*> in_ius{&input_dependency};
   std::vector<const IU*> out_ius_{&pointers_};
   std::vector<const IU*> args{};
   std::vector<bool> ref{};
   const IU* out = &pointers_;
   return std::unique_ptr<RuntimeFunctionSubop>(
      new RuntimeFunctionSubop(
         source,
         state_init_,
         std::move(fct_name),
         std::move(in_ius),
         std::move(out_ius_),
         std::move(args),
         std::move(ref),
         out));
}

RuntimeFunctionSubop::RuntimeFunctionSubop(
   const RelAlgOp* source,
   DefferredStateInitializer* state_init_,
   std::string fct_name_,
   std::vector<const IU*>
      in_ius_,
   std::vector<const IU*>
      out_ius_,
   std::vector<const IU*>
      args_,
   std::vector<bool>
      ref_,
   const IU* out_)
   : TemplatedSuboperator<RuntimeFunctionSubopState>(source, std::move(out_ius_), std::move(in_ius_)), state_init(state_init_), fct_name(std::move(fct_name_)), args(std::move(args_)), ref(std::move(ref_)), out(out_) {
}

void RuntimeFunctionSubop::consumeAllChildren(CompilationContext& context) {
   auto& builder = context.getFctBuilder();
   const auto& program = context.getProgram();

   // Get the object pointer from the backing global state.
   auto gstate_raw_ptr = context.accessGlobalState(*this);
   auto gstate_casted = IR::CastExpr::build(std::move(gstate_raw_ptr), IR::Pointer::build(program.getStruct(RuntimeFunctionSubopState::name)));
   auto hash_table_ptr = IR::StructAccessExpr::build(std::move(gstate_casted), "this_object");

   std::unordered_set<const IU*> provided;

   // Declare the output IUs.
   for (const IU* out_iu : provided_ius) {
      provided.emplace(out_iu);
      auto iu_name = context.buildIUIdentifier(*out_iu);
      const auto& declare = builder.appendStmt(IR::DeclareStmt::build(std::move(iu_name), out_iu->type));
      context.declareIU(*out_iu, declare);
   }

   // Assemble the input expressions.
   std::vector<IR::ExprPtr> args_exprs;
   // First argument is the runtime object, followup arguments are IUs.
   args_exprs.push_back(std::move(hash_table_ptr));
   for (size_t idx = 0; idx < args.size(); ++idx) {
      auto arg = args[idx];
      auto& declaration = context.getIUDeclaration(*arg);
      IR::ExprPtr arg_expr = IR::VarRefExpr::build(declaration);
      if (ref[idx]) {
         arg_expr = IR::CastExpr::build(IR::RefExpr::build(std::move(arg_expr)), IR::Pointer::build(IR::Char::build()));
      }
      if (provided.contains(arg)) {
         // The function actually wants a pointer to this argument to be able
         // to assign to the output IU.
         args_exprs.push_back(IR::RefExpr::build(std::move(arg_expr)));
      } else {
         // This argument is a regular input argument, we can pass the IU statement directly.
         args_exprs.push_back(std::move(arg_expr));
      }
   }

   // Call the function.
   auto runtime_fct = context.getRuntimeFunction(fct_name).get();

   // And build the assignment into the output IU.
   IR::ExprPtr invoke_expr =
      IR::InvokeFctExpr::build(*runtime_fct, std::move(args_exprs));
   IR::StmtPtr call_stmt;
   if (!out) {
      // We are not assigning the function result to something.
      call_stmt = IR::InvokeFctStmt::build(std::move(invoke_expr));
   } else {
      assert(provided.contains(out));
      // We are assigning the function result to one of the produced IUs.
      const auto& out_declare = context.getIUDeclaration(*out);
      call_stmt =
         IR::AssignmentStmt::build(
            out_declare, std::move(invoke_expr));
   }

   builder.appendStmt(std::move(call_stmt));

   if (!isSink()) {
      // And notify consumer that IUs are ready.
      context.notifyIUsReady(*this);
   }
}

void RuntimeFunctionSubop::setUpStateImpl(const ExecutionContext& context) {
   assert(state_init);
   state_init->prepare(context.getNumThreads());
   for (size_t thread_id = 0; thread_id < context.getNumThreads(); ++thread_id) {
      auto& state = (*states)[thread_id];
      state.this_object = state_init->access(thread_id);
   }
}

std::string RuntimeFunctionSubop::id() const {
   std::stringstream str;
   str << "rt_fct_" << fct_name;
   for (const IU* arg : args) {
      str << "_" << arg->type->id();
   }
   if (out) {
      str << "_" << out->type->id();
   }
   return str.str();
}

}
