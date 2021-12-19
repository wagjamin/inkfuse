#ifndef INKFUSE_OPERATOR_H
#define INKFUSE_OPERATOR_H

#include <memory>

namespace inkfuse {

    /// Central physical operator within inkfuse. These operators are then embedded within
    /// a pipeline for execution. The operators follow the traditional produce/consume model
    /// in order to generate code.
    struct Operator {

    };

    /// Leaf operator of a pipeline, serves as data source for that pipeline.
    /// Effectively always a table scan.
    struct Source: public Operator {

    };

    /// Sink operator of a pipeline, serves as data sink for that pipeline.
    /// Could be e.g. a print operator, or an aggregation or hash table sink.
    struct Sink: public Operator {

    };

    /// The FuseSource and FuseSink below represent vectorized staging points between queries.
    /// They enable passing vectorized chunks between different operators in the engine.
    /// Effectively, they generate a set of rows in a columnar in-memory layout.

    /// Incremental fusion source. Not part of the actual data pipelines being created for a query.
    /// Rather, this source operator is dynamically inserted when compiling a fraction of the query plan.
    /// Additionally, it is used as the sink for the vectorized fragments.
    struct FuseSource: public Source {

    };

    /// Incremental fusion sink. Not part of the actual data pipelines being created for a query.
    /// Rather, this sink operator is dynamically inserted when compiling a fraction of the query plan.
    /// Additionally, it is used as the sink for the vectorized fragments.
    struct FuseSink: public Sink {

    };

    using OperatorPtr = std::unique_ptr<Operator>;

}

#endif //INKFUSE_OPERATOR_H
