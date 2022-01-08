#ifndef INKFUSE_IRBUILDER_H
#define INKFUSE_IRBUILDER_H

#include "codegen/Types.h"

namespace inkfuse {

namespace IR {

    struct Program;
    struct Function;
    using FunctionArc = std::shared_ptr<Function>;

    struct FunctionBuilder {



    private:
        friend class IRBuilder;

        FunctionBuilder(Program& program_);

        /// Backing program for which we generate code.
        Program& program;
    };

    struct IRBuilder {
    public:
        /// Add a struct to the backing program.
        void addStruct(StructArc new_struct);

        /// Add an external function to teh backing program.
        void addExternalFunction(FunctionArc function);

        /// Create a function builder for the given function header.
        /// The body of the function should be empty.
        FunctionBuilder buildFunction(FunctionArc function);


    private:
        friend class Program;

        IRBuilder(Program& program_);

        /// Backing program for which we generate code.
        Program& program;
    };

}

};

#endif //INKFUSE_IRBUILDER_H
