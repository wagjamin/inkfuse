#ifndef INKFUSE_TPCH_H
#define INKFUSE_TPCH_H

#include "storage/Relation.h"
#include "algebra/Pipeline.h"
#include "algebra/RelAlgOp.h"
#include "algebra/Print.h"

/// TPCH schemas and supported queries.
namespace inkfuse::tpch {

/// Get the full TPC-H schema containing all relations with all columns.
Schema getTPCHSchema();

std::unique_ptr<Print> q1(const Schema& schema);
std::unique_ptr<Print> q6(const Schema& schema);

/// Some interesting custom queries. See /tpch for query text.
std::unique_ptr<Print> l_count(const Schema& schema);
std::unique_ptr<Print> l_point(const Schema& schema);


}

#endif //INKFUSE_TPCH_H