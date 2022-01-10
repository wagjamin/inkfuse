#include "codegen/IR.h"

namespace inkfuse {

namespace IR {

    /// Set up the global runtime. Structs and functions are added by the respective.
    ProgramArc global_runtime = std::make_shared<Program>("global_runtime");

    Function::Function(std::string name_, std::vector<StmtPtr> arguments_, TypeArc return_type_)
       : name(std::move(name_)), arguments(std::move(arguments_)), return_type(std::move(return_type_))
    {
       for (const auto& ptr: arguments) {
          if (!dynamic_cast<DeclareStmt*>(ptr.get())) {
             throw std::runtime_error("function must have declare statements as arguments");
          }
       }
    }

    const Block *Function::getBody()
    {
        return body.get();
    }

    Program::Program(std::string program_name_, bool standalone): program_name(std::move(program_name_))
    {
        if (!standalone) {
            // We depend on the global runtime by default.
            includes.push_back(global_runtime);
        }
    }

    IRBuilder Program::getIRBuilder()
    {
       return IRBuilder(*this);
    }

    const std::vector<ProgramArc> &Program::getIncludes() const
    {
        return includes;
    }

    const std::vector<StructArc> &Program::getStructs() const
    {
        return structs;
    }

    const std::vector<FunctionArc> &Program::getFunctions() const
    {
        return functions;
    }
}

}
