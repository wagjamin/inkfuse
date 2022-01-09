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
        for (const auto& structure: program.getStructs()) {
            compileStruct(*structure, writer);
        }

        // Step 3: Set up the functions used throughout the program.
        for (const auto& function: program.getFunctions()) {
            compileFunction(*function, writer);
        }

        return std::make_unique<BackendProgramC>(writer.str());
    }

    void BackendC::createPreamble(ScopedWriter& writer)
    {

    }

    void BackendC::typeDescription(const IR::Type& type, ScopedWriter::Statement& str)
    {
    struct DescriptionVisitor final : public IR::TypeVisitor<ScopedWriter::Statement&> {
        private:
            void visitSignedInt(const IR::SignedInt& type, ScopedWriter::Statement& arg) override
            {
                arg.stream() << "int" << 8 * type.numBytes() << "_t";
            }

            void visitUnsignedInt(const IR::UnsignedInt& type, ScopedWriter::Statement& arg) override
            {
                arg.stream() << "uint" << 8 * type.numBytes() << "_t";
            }

            void visitBool(const IR::Bool& type, ScopedWriter::Statement& arg) override
            {
                arg.stream() << "bool";
            }
        };

        DescriptionVisitor visitor;
        visitor.visit(type, str);
    }

    void BackendC::compileStruct(const IR::Struct &structure, ScopedWriter& str)
    {

    }

    void BackendC::compileFunction(const IR::Function &fct, ScopedWriter& str)
    {

    }

    void BackendC::compileBlock(const IR::Block &block, ScopedWriter& str)
    {

    }

    void BackendC::compileStatement(const IR::Stmt &statement, ScopedWriter& str)
    {

    }

    void BackendC::compileExpression(const IR::Expr &expr, ScopedWriter& str)
    {

    }
}