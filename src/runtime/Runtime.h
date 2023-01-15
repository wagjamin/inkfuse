#ifndef INKFUSE_RUNTIME_H
#define INKFUSE_RUNTIME_H

#include "codegen/IR.h"
#include "codegen/Type.h"

namespace inkfuse {

    struct GlobalRuntime {
        /// Constructor sets up the global runtime.
        GlobalRuntime();

        /// Global runtime IR program.
        IR::ProgramArc program;
    };

    /// The inkfuse runtime. A global program containing all structs and functions exposed by the runtime system.
    /// The runtime has to be made accessible from every program which has to access the runtime
    /// in some reasonable way. It is added to an include to every non-standalone program.
    extern GlobalRuntime global_runtime;

    /// Build a struct to be made accessible in the inkfuse runtime.
    struct RuntimeStructBuilder {

        /// Create a runtime struct builder.
        RuntimeStructBuilder(std::string name_): name(std::move(name_)) {};

        /// Drop the runtime struct builder - adds the struct to the global runtime.
        ~RuntimeStructBuilder();

        /// Add a new member with the given name to the struct.
        RuntimeStructBuilder& addMember(std::string member_name, IR::TypeArc type);


    private:
        /// Struct name.
        std::string name;
        /// Fields of the struct.
        std::vector<IR::Struct::Field> fields;
    };

    /// Build a function to be made accessible in the inkfuse runtime.
    struct RuntimeFunctionBuilder {

        /// Create a new runtime function builder.
        RuntimeFunctionBuilder(std::string name_, IR::TypeArc return_type): name(std::move(name_)), return_type(std::move(return_type)) {}

        /// Drop the runtime function builder - adds the function to the global runtime.
        ~RuntimeFunctionBuilder();

        /// Add a new argument with the given name to the function.
        RuntimeFunctionBuilder& addArg(std::string arg_name, IR::TypeArc type, bool const_arg = false);

    private:
        /// Function name.
        std::string name;
        /// Return type of function being built.
        IR::TypeArc return_type;
        /// Arguments of the function being built.
        std::vector<IR::StmtPtr> arguments;
        /// Which arguments are const?
        std::vector<bool> is_const;
    };

}

#endif //INKFUSE_RUNTIME_H
