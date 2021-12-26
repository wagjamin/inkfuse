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
        ScopedWriter writer;

        // Step 1: Set up the preamble containing includes.
        createPreamble(writer);

        // Step 2: Set up the structs used throughout the program.
        for (const auto& structure: program.structures) {
            compileStruct(*structure, writer);
        }

        // Step 3: Set up the functions used throughout the program.
        for (const auto& function: program.functions) {
            compileFunction(*function, writer);
        }

        return std::make_unique<BackendProgramC>(writer.str());
    }

    void BackendC::createPreamble(std::stringstream &str)
    {

    }

    void BackendC::typeDescription(const IR::Type& type, std::stringstream& str)
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

    void BackendC::compileStruct(const IR::Structure &structure, std::stringstream &str)
    {

    }

    void BackendC::compileFunction(const IR::Function &fct, std::stringstream &str)
    {

    }

    void BackendC::compileBlock(const IR::Block &block, std::stringstream &str)
    {

    }

    void BackendC::compileStatement(const IR::Stmt &statement, std::stringstream &str)
    {

    }

    void BackendC::compileExpression(const IR::Expr &expr, std::stringstream &str)
    {

    }
}