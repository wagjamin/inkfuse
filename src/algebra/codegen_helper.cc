#include "imlab/algebra/codegen_helper.h"
// ---------------------------------------------------------------------------
namespace imlab {
// ---------------------------------------------------------------------------
ScopedBlock::~ScopedBlock()
{
    helper->StopIndentBlock();
    helper->RawStream() << indent_str << "}\n";
}
// ---------------------------------------------------------------------------
ScopedBlock::ScopedBlock(CodegenHelper &helper_, std::string indent_str_): helper(&helper_), indent_str(std::move(indent_str_))
{
    helper->RawStream() << indent_str << "{\n";
    helper->StartIndentBlock();
}
// ---------------------------------------------------------------------------
Statement::Statement(std::ostream &out_, const std::string& indent_str, bool terminate_semicolon_): out(out_), terminate_semicolon(terminate_semicolon_)
{
    out << indent_str;
}
// ---------------------------------------------------------------------------
Statement & Statement::operator<<(const std::string &str)
{
    out << str;
    return *this;
}
// ---------------------------------------------------------------------------
Statement::~Statement()
{
    if (terminate_semicolon) {
        out << ";\n";
    } else {
        out << "\n";
    }
}
// ---------------------------------------------------------------------------
CodegenHelper::CodegenHelper(std::ostream* out_): out(out_)
{
}
// ---------------------------------------------------------------------------
Statement CodegenHelper::Stmt()
{
    CheckReady();
    return Statement(*out, indent_str, true);
}
// ---------------------------------------------------------------------------
Statement CodegenHelper::Flow()
{
    CheckReady();
    return Statement(*out, indent_str, false);
}
// ---------------------------------------------------------------------------
ScopedBlock CodegenHelper::CreateScopedBlock()
{
    CheckReady();
    return ScopedBlock{*this, indent_str};
}
// ---------------------------------------------------------------------------
void CodegenHelper::StartIndentBlock()
{
    indent_level += 4;
    RebuildIndentStr();
}
// ---------------------------------------------------------------------------
void CodegenHelper::StopIndentBlock()
{
    indent_level -= 4;
    RebuildIndentStr();
}
// ---------------------------------------------------------------------------
std::ostream & CodegenHelper::RawStream()
{
    CheckReady();
    return *out;
}
// ---------------------------------------------------------------------------
void CodegenHelper::RebuildIndentStr()
{
    indent_str = std::string(indent_level, ' ');
}
// ---------------------------------------------------------------------------
void CodegenHelper::AttachOutputStream(std::ostream& ostream)
{
    if (out) {
        throw std::runtime_error("Output stream can only be attached once to CodegenHelper.");
    }
    out = &ostream;
}
// ---------------------------------------------------------------------------
void CodegenHelper::CheckReady()
{
    if (!out) {
        throw std::runtime_error("Missing ostream in CodegenHelper");
    }
}
// ---------------------------------------------------------------------------
} // namespace imlab
// ---------------------------------------------------------------------------
