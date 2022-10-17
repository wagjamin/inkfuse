#ifndef INKFUSE_TPCH_H
#define INKFUSE_TPCH_H

#include "storage/Relation.h"
#include "algebra/Pipeline.h"
#include "algebra/RelAlgOp.h"

/// TPCH schemas and supported queries.
namespace inkfuse::tpch {

/// Get the full TPC-H schema containing all relations with all columns.
Schema getTPCHSchema();

RelAlgOpPtr q1(const Schema& schema);

}

#endif //INKFUSE_TPCH_H