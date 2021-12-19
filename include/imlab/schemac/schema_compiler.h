// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_SCHEMAC_SCHEMA_COMPILER_H_
#define INCLUDE_IMLAB_SCHEMAC_SCHEMA_COMPILER_H_
// ---------------------------------------------------------------------------------------------------
#include "imlab/schemac/schema_parse_context.h"
#include <ostream>
// ---------------------------------------------------------------------------------------------------
namespace imlab {
namespace schemac {
// ---------------------------------------------------------------------------------------------------
class SchemaCompiler {
 public:
    // Constructor
    SchemaCompiler(std::ostream &header_, std::ostream &impl_, std::string header_path_)
        : header(header_), impl(impl_), header_path(std::move(header_path_)) {}
    // Compile a schema
    void Compile(Schema &schema);

 private:
    /// Build the backing relation struct.
    void writeRelationStruct(const Schema & schema);

    /// Write out the backing CRUD functions.
    void writeCRUD(const Schema & schema);

    /// Write out the schema preeamble.
    void writePreeamble(const Schema & schema);

    /// Write out the schema postamble (not a word, I know).
    void writePostamble(const Schema & schema);

    // Path to the header file.
    std::string header_path;
    // Output stream for the header
    std::ostream &header;
    // Output stream for the implementation
    std::ostream &impl;
};
// ---------------------------------------------------------------------------------------------------
}  // namespace schemac
}  // namespace imlab
// ---------------------------------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_SCHEMAC_SCHEMA_COMPILER_H_
// ---------------------------------------------------------------------------------------------------

