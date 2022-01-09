#include "codegen/IR.h"

namespace inkfuse {

namespace IR {

    /// Set up the global runtime. Structs and functions are added by the respective.
    ProgramArc global_runtime = std::make_shared<Program>("global_runtime");

    Function::Function(std::string name_, std::vector<TypeArc> arguments_): name(std::move(name_)), arguments(std::move(arguments_))
    {
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
