#ifndef INKFUSE_TPCH_H
#define INKFUSE_TPCH_H

#include "storage/Relation.h"

/// TPCH schemas and supported queries.
namespace inkfuse::tpch {

/// Get the full TPC-H schema containing all relations with all columns.
Schema getTPCHSchema();

}

#endif //INKFUSE_TPCH_H