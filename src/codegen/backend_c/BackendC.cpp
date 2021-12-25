#include "codegen/backend_c/BackendC.h"

namespace inkfuse {

    void BackendProgramC::compileToMachinecode()
    {
        // TODO
    }

    void* BackendProgramC::getFunction(std::string_view name)
    {
        // TODO
        return nullptr;
    }

    void BackendProgramC::dump()
    {
        // TODO
    }

    std::unique_ptr<IR::BackendProgram> BackendC::generate(const IR::Program &program) const
    {
        std::stringstream stream;
        return std::make_unique<BackendProgramC>(stream.str());
    }

    void BackendC::typeDescription(const IR::Type& type, std::stringstream str)
    {
        struct DescriptionVisitor final : public IR::TypeVisitor<std::stringstream&> {
        private:
            void visitSignedInt(const IR::SignedInt& type, std::stringstream& arg) override
            {
                arg << "int" << 8 * type.numBytes() << "_t";
            }

            void visitUnsignedInt(const IR::UnsignedInt& type, std::stringstream& arg) override
            {
                arg << "uint" << 8 * type.numBytes() << "_t";
            }

            void visitBool(const IR::Bool& type, std::stringstream& arg) override
            {
                arg << "bool";
            }
        };

        DescriptionVisitor visitor;
        visitor.visit(type, str);
    }

}