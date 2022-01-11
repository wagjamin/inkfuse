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

    ScopedWriter::Statement::Statement(ScopedWriter &writer_, bool semicolon_): writer(writer_), semicolon(semicolon_)
    {
        writer.stream << writer.indent_string;
    }

    std::stringstream &ScopedWriter::Statement::stream()
    {
        return writer.stream;
    }

    ScopedWriter::Statement::~Statement()
    {
       if (semicolon) {
          writer.stream << ";\n";
       } else {
          writer.stream << "\n";
       }
    }

    std::string ScopedWriter::str() const
    {
        return stream.str();
    }

    ScopedWriter::ScopedBlock ScopedWriter::block()
    {
        return ScopedBlock{*this};
    }

    ScopedWriter::Statement ScopedWriter::stmt(bool semicolon)
    {
        return Statement{*this, semicolon};
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
