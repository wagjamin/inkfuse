#include "codegen/backend_c/ScopedWriter.h"

namespace inkfuse {

    ScopedWriter::ScopedBlock::ScopedBlock(ScopedWriter &writer_): writer(writer_)
    {
        writer.stream << writer.indent_string << "{\n";
        writer.indent();
    }

    ScopedWriter::ScopedBlock::~ScopedBlock()
    {
        writer.unindent();
        writer.stream << writer.indent_string << "}\n";
    }

    ScopedWriter::Statement::Statement(ScopedWriter &writer_): writer(writer_)
    {
        writer.stream << writer.indent_string;
    }

    ScopedWriter::Statement::~Statement()
    {
        writer.stream << ";\n";
    }

    std::string ScopedWriter::str() const
    {
        return stream.str();
    }

    ScopedWriter::ScopedBlock ScopedWriter::block()
    {
        return ScopedBlock{*this};
    }

    ScopedWriter::Statement ScopedWriter::stmt()
    {
        return Statement{*this};
    }

    void ScopedWriter::indent()
    {
        indent_level += 4;
        indent_string = std::string(indent_level, ' ');
    }

    void ScopedWriter::unindent()
    {
        indent_level -= 4;
        indent_string = std::string(indent_level, ' ');
    }

}
