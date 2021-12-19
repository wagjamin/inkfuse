// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_ALGEBRA_IU_H_
#define INCLUDE_IMLAB_ALGEBRA_IU_H_
// ---------------------------------------------------------------------------
#include "imlab/schemac/schema_parse_context.h"
// ---------------------------------------------------------------------------
namespace imlab {
// ---------------------------------------------------------------------------
// Information unit
struct IU {
    // Table name
    const char *table;
    // Column name
    const char *column;
    // Type of the IU
    const schemac::Type type;

    // Get the variable name of this IU.
    std::string Varname() const;

    // Anonymous IU constructor
    IU(const char *column, const schemac::Type type)
        : table(nullptr), column(column), type(type) {}
    // Table IU constructor
    IU(const char *table, const char *column, const schemac::Type type)
        : table(table), column(column), type(type) {}
};
// ---------------------------------------------------------------------------
}  // namespace imlab
// ---------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_ALGEBRA_IU_H_
// ---------------------------------------------------------------------------

