#ifndef IMLAB_CODEGEN_HELPER_H
#define IMLAB_CODEGEN_HELPER_H
// ---------------------------------------------------------------------------
#include <optional>
#include <ostream>
#include <string>
// ---------------------------------------------------------------------------
namespace imlab {
// ---------------------------------------------------------------------------
struct CodegenHelper;
// ---------------------------------------------------------------------------
struct ScopedBlock {
    // Destructor terminating the scoped block.
    ~ScopedBlock();

private:
    friend class CodegenHelper;

    explicit ScopedBlock(CodegenHelper& helper_, std::string indent_str);

    CodegenHelper* helper;
    std::string indent_str;
};
// ---------------------------------------------------------------------------
struct Statement {
public:
    // Custom print operator respecting indentation.
    Statement& operator<<(const std::string& str);

    // Destructor terminating the statement.
    ~Statement();

private:
    Statement(std::ostream& out_, const std::string& indent_str, bool terminate_semicolon_);

    friend class CodegenHelper;
    // Output stream wrapped by the helper.
    std::ostream& out;
    // Should the statement be termianted by a semicolon?
    bool terminate_semicolon;
};
// ---------------------------------------------------------------------------
/// Helper struct for easy codegen.
struct CodegenHelper {
public:
    explicit CodegenHelper(std::ostream* out_ = nullptr);

    /// Start a new block with increased indentation.
    void StartIndentBlock();
    /// Start a new block with increased indentation.
    void StopIndentBlock();

    /// Create a new statement on the current indentation level.
    Statement Stmt();

    /// Create a new control flow block on current indentation level.
    Statement Flow();

    /// Create a new scoped block.
    ScopedBlock CreateScopedBlock();

    /// Get the raw stream.
    std::ostream& RawStream();

    /// Attach an output stream.
    void AttachOutputStream(std::ostream& ostream);

private:
    // Output stream wrapped by the helper.
    std::ostream* out;
    // Current indentation level.
    size_t indent_level = 0;
    // Current indentation string.
    std::string indent_str = "";

    void RebuildIndentStr();

    /// Ensure that the helper has an output stream attached.
    void CheckReady();
};
// ---------------------------------------------------------------------------
}  // namespace imlab
// ---------------------------------------------------------------------------
#endif //IMLAB_CODEGEN_HELPER_H
