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

/// We implement the physical plans for a subset of interesting TPC-H
/// queries that allows for representative benchmarking.
/// Physical plans are taken from Umbra: https://umbra-db.com/interface/

/// Low-Cardinality Aggregation (4 groups)
std::unique_ptr<Print> q1(const Schema& schema);
/// Join (small build, big probe) ~20x difference
std::unique_ptr<Print> q3(const Schema& schema);
/// Join (very small build, big probe) ~70x difference
std::unique_ptr<Print> q4(const Schema& schema);
/// Selective filters
std::unique_ptr<Print> q6(const Schema& schema);
/// Join (moderate build, moderate probe) ~5x difference
std::unique_ptr<Print> q9(const Schema& schema);
/// High-Cardinality aggregation (1.5M groups on SF1)
std::unique_ptr<Print> q18(const Schema& schema);

/// Some interesting custom queries. See /tpch for query text.
std::unique_ptr<Print> l_count(const Schema& schema);
std::unique_ptr<Print> l_point(const Schema& schema);


}

#endif //INKFUSE_TPCH_H