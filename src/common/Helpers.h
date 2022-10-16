#ifndef INKFUSE_HELPERS_H
#define INKFUSE_HELPERS_H

#include "storage/Relation.h"
#include <cstdint>
#include <string>

/// Common helper functions used throuhgout different parts of InkFuse.
namespace inkfuse::helpers {

/// Transform a date literal into an integer represented in the runtime.
/// The date is stored as offset to the epoch 1-1-1970.
int32_t dateStrToInt(const char* str);

/// Load data into the backing columns of a schema.
/// Looks for '|' separated .tbl files within the directory of `path`.
void loadDataInto(Schema& schema, std::string path);

} // namespace inkfuse

#endif