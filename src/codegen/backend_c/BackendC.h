#ifndef INKFUSE_BACKENDC_H
#define INKFUSE_BACKENDC_H

#include <memory>
#include <string>
#include <sstream>
#include "codegen/Backend.h"
#include "codegen/Types.h"

namespace inkfuse {

    /// Backend program in C.
    struct BackendProgramC : public IR::BackendProgram {

        BackendProgramC(std::string program_): program(std::move(program_)) {};

        ~BackendProgramC() override = default;

        /// Compile the backend program to actual machine code.
        void compileToMachinecode() override;

        /// Get a function with the specified name from the compiled program.
        void* getFunction(std::string_view name) override;

        /// Dump the backend program in a readable way into a file.
        void dump() override;

    private:
        /// Was this program compiled already?
        bool was_compiled = false;
        /// The actual program which is simply generated C stored within a backing string.
        const std::string program;
    };

    /// Simplge C backend translating the inkfuse IR to plain C.
    struct BackendC : public IR::Backend {

        ~BackendC() override = default;

        /// Generate a backend program from the high-level IR.
        std::unique_ptr<IR::BackendProgram> generate(const IR::Program& program) const override;

    private:

        /// Add a type description to the backing string stream.
        static void typeDescription(const IR::Type& type, std::stringstream str);

    };

}

#endif //INKFUSE_BACKENDC_H
