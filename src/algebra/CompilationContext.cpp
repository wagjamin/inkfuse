#include "algebra/CompilationContext.h"
#include "algebra/Pipeline.h"
#include "algebra/suboperators/Suboperator.h"
#include <iostream>
#include <sstream>

namespace inkfuse {

CompilationContext::CompilationContext(std::string program_name, const Pipeline& pipeline_)
   : pipeline(pipeline_), program(std::make_shared<IR::Program>(std::move(program_name), false)), fct_name("execute") {
}

CompilationContext::CompilationContext(IR::ProgramArc program_, std::string fct_name_, const Pipeline& pipeline_)
   : pipeline(pipeline_), program(std::move(program_)), fct_name(std::move(fct_name_)) {
}

void CompilationContext::compile() {
   // Create the builder.
   builder.emplace(*program, fct_name);
   // Collect all sinks.
   std::set<Suboperator*> sinks;
   std::for_each(pipeline.suboperators.begin(), pipeline.suboperators.end(), [&](const SuboperatorArc& op) {
      if (op->isSink()) {
         sinks.insert(op.get());
      }
   });
   // Open all sinks.
   for (auto sink : sinks) {
      sink->open(*this);
   }
   // And close them again.
   for (auto sink : sinks) {
      sink->close(*this);
   }
   // Return 0 for the time being.
   builder->fct_builder.appendStmt(IR::ReturnStmt::build(IR::ConstExpr::build(IR::UI<1>::build(0))));
   // Destroy the builders.
   builder.reset();
}

void CompilationContext::notifyOpClosed(Suboperator& op) {
   for (auto producer : pipeline.getProducers(op)) {
      if (--properties[producer].upstream_requests == 0) {
         // This was the last upstream operator to close. Close the child as well.
         producer->close(*this);
      }
   }
}

void CompilationContext::notifyIUsReady(Suboperator& op) {
   // Fetch the sub-operator which requested the given IU.
   computed.emplace(&op);
   auto [requestor, iu] = requests[&op];
   // Remove the now serviced request from the map again.
   requests.erase(&op);
   assert(requestor);
   // Consume in the original requestor.
   requestor->consume(*iu, *this);
   if (++properties[requestor].serviced_requests == requestor->getNumSourceIUs()) {
      // Consume in the original requestor notifying it that all children were produced successfuly.
      requestor->consumeAllChildren(*this);
   }
}

void CompilationContext::requestIU(Suboperator& op, const IU& iu) {
   // Resolve the IU provider.
   bool found = false;
   for (auto provider : pipeline.getProducers(op)) {
      const auto& provided = provider->getIUs();
      if (std::find(provided.cbegin(), provided.cend(), &iu) != provided.cend()) {
         properties[provider].upstream_requests++;
         requests[provider] = {&op, &iu};
         if (!computed.count(provider)) {
            // Request to compute if it was not computed yet.
            provider->open(*this);
         } else {
            // Otherwise directly notify the parent that we are ready.
            notifyIUsReady(*provider);
         }
         found = true;
         break;
      }
   }
   if (!found) {
      throw std::runtime_error("No child produces the IU.");
   }
}

void CompilationContext::declareIU(const IU& iu, const IR::Stmt& stmt) {
   assert(!iu_declarations.contains(&iu));
   assert(dynamic_cast<const IR::DeclareStmt*>(&stmt));
   iu_declarations[&iu] = &stmt;
}

std::string CompilationContext::buildIUIdentifier(const IU& iu) {
   if (!iu.name.empty()) {
      return "iu_" + iu.name;
   } else {
      std::stringstream iu_stream;
      iu_stream << "iu_" << &iu;
      return iu_stream.str();
   }
}

const IR::Stmt& CompilationContext::getIUDeclaration(const IU& iu) {
   return *iu_declarations.at(&iu);
}

IR::ExprPtr CompilationContext::accessGlobalState(const Suboperator& op) const {
   assert(builder);
   // Get global state.
   auto& global_state = builder->fct_builder.getArg(0);
   auto found = std::find_if(
      pipeline.suboperators.cbegin(),
      pipeline.suboperators.cend(),
      [&op](const SuboperatorArc& ptr) {
         return ptr.get() == &op;
      });
   auto idx = std::distance(pipeline.suboperators.cbegin(), found);
   // Add the state offset to the pointer.
   auto offset = IR::ArithmeticExpr::build(
      IR::VarRefExpr::build(global_state),
      IR::ConstExpr::build(IR::UI<4>::build(idx)),
      IR::ArithmeticExpr::Opcode::Add);
   return IR::DerefExpr::build(std::move(offset));
}

IR::FunctionArc CompilationContext::getRuntimeFunction(std::string_view name) const {
   assert(program->getIncludes().size() == 1);
   auto& include = program->getIncludes()[0];
   return include->getFunction(name);
}

const IR::Program& CompilationContext::getProgram() {
   return *program;
}

IR::FunctionBuilder& CompilationContext::getFctBuilder() {
   return builder->fct_builder;
}

CompilationContext::Builder::Builder(IR::Program& program, std::string fct_name)
   : ir_builder(program.getIRBuilder()), fct_builder(createFctBuilder(ir_builder, std::move(fct_name))) {
}

IR::FunctionBuilder CompilationContext::createFctBuilder(IR::IRBuilder& program, std::string fct_name) {
   // Generated functions for execution have two void** arguments for state
   // and a void* for the resumption state.
   static const std::vector<std::pair<std::string, IR::TypeArc>> arg_names{
      {"global_state", IR::Pointer::build(IR::Pointer::build(IR::Void::build()))},
      {"params", IR::Pointer::build(IR::Pointer::build(IR::Void::build()))},
      {"resumption_state", IR::Pointer::build(IR::Void::build())},
   };
   std::vector<IR::StmtPtr> args;
   for (const auto& [arg, type] : arg_names) {
      args.push_back(IR::DeclareStmt::build(arg, type));
   }
   // Return 1 byte integer which can be cast to a yield-state enum.
   auto return_type = IR::UnsignedInt::build(1);
   return program.createFunctionBuilder(std::make_shared<IR::Function>(std::move(fct_name), std::move(args), std::move(return_type)));
}

}
