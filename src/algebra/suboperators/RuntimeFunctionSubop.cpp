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

std::unique_ptr<RuntimeFunctionSubop> RuntimeFunctionSubop::htLookup(const RelAlgOp* source, const IU& pointers_, const IU& keys_, std::vector<const IU*> pseudo_ius_, void* hash_table) {
   std::string fct_name = "ht_sk_lookup";
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
         std::move(fct_name),
         std::move(in_ius),
         std::move(out_ius_),
         std::move(args),
         std::move(ref),
         out,
         hash_table));
}

std::unique_ptr<RuntimeFunctionSubop> RuntimeFunctionSubop::htLookupOrInsert(const RelAlgOp* source, const IU& pointers_, const IU& keys_, std::vector<const IU*> pseudo_ius_, void* hash_table_) {
   std::string fct_name = "ht_sk_lookup_or_insert";
   std::vector<const IU*> in_ius{&keys_};
   for (auto pseudo : pseudo_ius_) {
      // Pseudo IUs are used as input IUs in the backing graph, but do not influence arguments.
      in_ius.push_back(pseudo);
   }
   // The argument needs to be referenced if we directly use a non-packed IU as argument.
   std::vector<bool> ref{keys_.type->id() != "ByteArray" && keys_.type->id() != "Ptr_Char"};
   std::vector<const IU*> out_ius_{&pointers_};
   std::vector<const IU*> args{&keys_};
   const IU* out = &pointers_;
   return std::unique_ptr<RuntimeFunctionSubop>(
      new RuntimeFunctionSubop(
         source,
         std::move(fct_name),
         std::move(in_ius),
         std::move(out_ius_),
         std::move(args),
         std::move(ref),
         out,
         hash_table_));
}

RuntimeFunctionSubop::RuntimeFunctionSubop(
   const RelAlgOp* source,
   std::string fct_name_,
   std::vector<const IU*>
      in_ius_,
   std::vector<const IU*>
      out_ius_,
   std::vector<const IU*>
      args_,
   std::vector<bool>
      ref_,
   const IU* out_,
   void* this_object_)
   : TemplatedSuboperator<RuntimeFunctionSubopState>(source, std::move(out_ius_), std::move(in_ius_)), fct_name(std::move(fct_name_)), args(std::move(args_)), ref(std::move(ref_)), out(out_), this_object(this_object_) {
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
         arg_expr = IR::RefExpr::build(std::move(arg_expr));
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

   // And notify consumer that IUs are ready.
   context.notifyIUsReady(*this);
}

void RuntimeFunctionSubop::setUpStateImpl(const ExecutionContext& context) {
   state->this_object = static_cast<void*>(this_object);
}

std::string RuntimeFunctionSubop::id() const {
   std::stringstream str;
   str << "rt_fct_" << fct_name;
   for (const IU* arg : args) {
      str << "_" << arg->type->id();
   }
   return str.str();
}

}