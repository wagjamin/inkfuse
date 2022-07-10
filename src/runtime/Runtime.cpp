#include "runtime/Runtime.h"
#include "algebra/suboperators/IndexedIUProvider.h"
#include "algebra/suboperators/LoopDriver.h"
#include "algebra/suboperators/expressions/RuntimeExpressionSubop.h"
#include "algebra/suboperators/sinks/CountingSink.h"
#include "algebra/suboperators/sinks/FuseChunkSink.h"
#include "runtime/HashRuntime.h"

namespace inkfuse {

/// Set up the global runtime. Structs and functions are added by the respective runtime helpers.
GlobalRuntime global_runtime = GlobalRuntime();

GlobalRuntime::GlobalRuntime() : program(std::make_unique<IR::Program>("global_runtime", true)) {
   // Register the different runtime structs of the suboperators.
   LoopDriverRuntime::registerRuntime();
   IndexedIUProviderRuntime::registerRuntime();
   FuseChunkSink::registerRuntime();
   CountingSink::registerRuntime();
   HashRuntime::registerRuntime();
   RuntimeExpressionSubop::registerRuntime();
}

RuntimeStructBuilder::~RuntimeStructBuilder() {
   global_runtime.program->getIRBuilder().addStruct(std::make_shared<IR::Struct>(std::move(name), std::move(fields)));
}

RuntimeStructBuilder& RuntimeStructBuilder::addMember(std::string member_name, IR::TypeArc type) {
   fields.push_back(IR::Struct::Field{.type = std::move(type), .name = std::move(member_name)});
   return *this;
}

RuntimeFunctionBuilder::~RuntimeFunctionBuilder() {
   global_runtime.program->getIRBuilder().addFunction(std::make_shared<IR::Function>(std::move(name), std::move(arguments), std::move(return_type)));
}

RuntimeFunctionBuilder& RuntimeFunctionBuilder::addArg(std::string arg_name, IR::TypeArc type) {
   arguments.push_back(IR::DeclareStmt::build(std::move(arg_name), std::move(type)));
   return *this;
}

}
