#include "algebra/CompilationContext.h"
#include "algebra/Pipeline.h"
#include "algebra/suboperators/Suboperator.h"

namespace inkfuse {

CompilationContext::CompilationContext(std::string program_name, Pipeline& pipeline_)
   : pipeline(pipeline_), program(std::move(program_name), false) {
}

void CompilationContext::compile() {
   // Create the builders.
   builder.emplace(program);
   for (const auto sink_offset : pipeline.sinks) {
      pipeline.suboperators[sink_offset]->produce(*this);
   }
   // Destroy the builders.
   builder.reset();
}

size_t CompilationContext::resolveScope(const Suboperator& op) {
   // TODO HACK
   return 0;
   // return pipeline.resolveOperatorScope(op);
}

void CompilationContext::notifyIUsReady(const Suboperator& op) {
   // Fetch the sub-operator which requested the given IU.
   computed.emplace(&op);
   auto [requestor, iu] = requests[&op];
   // Remove the now serviced request from the map again.
   requests.erase(&op);
   assert(requestor);
   // Consume in the original requestor.
   requestor->consume(*iu, *this);
   if (++serviced_requests[&op] == requestor->getNumSourceIUs()) {
      // Consume in the original requestor notifying it that all children were produced successfuly.
      requestor->consumeAllChildren(*this);
   }
}

void CompilationContext::requestIU(const Suboperator& op, IUScoped iu) {
   // Resolve the IU provider.
   Suboperator& provider = pipeline.getProvider(iu);
   // Store in the request map.
   requests[&provider] = {&op, &iu.iu};
   if (!computed.count(&provider)) {
      // Request to compute if it was not computed yet.
      provider.produce(*this);
   } else {
      // Otherwise directly notify the parent that we are ready.
      notifyIUsReady(provider);
   }
}

void CompilationContext::declareIU(IUScoped iu, const IR::Stmt& stmt) {
   scoped_declarations[{&iu.iu, iu.scope_id}] = &stmt;
}

const IR::Stmt& CompilationContext::getIUDeclaration(IUScoped iu) {
   return *scoped_declarations.at({&iu.iu, iu.scope_id});
}

IR::ExprPtr CompilationContext::accessGlobalState(const Suboperator& op) {
   assert(builder);
   // Get global state.
   auto& global_state = builder->fct_builder.getArg(0);
   auto found = std::find_if(
      pipeline.suboperators.cbegin(),
      pipeline.suboperators.cend(),
      [&op](const SuboperatorPtr& ptr) {
         return ptr.get() == &op;
      });
   auto idx = std::distance(pipeline.suboperators.cbegin(), found);
   // Add the state offset to the pointer.
   return IR::ArithmeticExpr::build(
      IR::VarRefExpr::build(global_state),
      IR::ConstExpr::build(IR::UI<4>::build(idx)),
      IR::ArithmeticExpr::Opcode::Add);
}

const IR::Program& CompilationContext::getProgram() {
   return program;
}

IR::FunctionBuilder& CompilationContext::getFctBuilder() {
   return builder->fct_builder;
}

CompilationContext::Builder::Builder(IR::Program& program)
   : ir_builder(program.getIRBuilder()), fct_builder(createFctBuilder(ir_builder))
{
}

IR::FunctionBuilder CompilationContext::createFctBuilder(IR::IRBuilder& program) {
   // Generated functions for execution have three void* arguments.
   static const std::vector<std::string> arg_names{
      "global_state",
      "params",
      "resumption_state"};
   std::vector<IR::StmtPtr> args;
   for (const auto& arg : arg_names) {
      args.push_back(IR::DeclareStmt::build(arg, IR::Pointer::build(IR::Void::build())));
   }
   /// Return 1 byte integer which can be cast to a yield-state enum.
   auto return_type = IR::UnsignedInt::build(1);
   return program.createFunctionBuilder(std::make_shared<IR::Function>("execute", std::move(args), std::move(return_type)));
}

}
