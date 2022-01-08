#ifndef INKFUSE_COLUMNSCAN_H
#define INKFUSE_COLUMNSCAN_H

#include "codegen/Types.h"
#include "codegen/Value.h"
#include "algebra/Suboperator.h"
#include "codegen/Expression.h"
#include "storage/Relation.h"

namespace inkfuse {

    /// State needed to get a table scan to operate on underlying data. Provided at runtime.
    struct ColumnScanState {
        /// Raw pointer to the column start.
        const void* data;
        /// Raw point to a potential selection byte mask.
        /// TODO use in a reasonable way.
        const bool* sel;
        /// Backing size of the column to read from.
        const uint64_t size;
    };

    /// Parameters needed to generate code for a column scan.
    struct ColumnScanParameters {
        /// Type being read by the table scan.
        IR::TypeArc type;
    };

    /// Scans a single column from either the underlying storage engine, or a fuse chunk.
    struct ColumnScan : public Suboperator {
    public:
        /// Set up a new column scan.
        ColumnScan(ColumnScanParameters params_, IURef provided_iu_, const BaseColumn& column_);

        void compile(CompilationContext& context, const std::set<IURef>& required) override;

        void setUpState() override;

        void tearDownState() override;

        std::string id() const override;

    private:
        /// Backing parameters.
        ColumnScanParameters params;
        /// IU provided by this scan.
        IURef provided_iu;
        /// Backing column to read from.
        const BaseColumn& column;
        /// Backing runtime state for the compiled code.
        std::unique_ptr<ColumnScanState> state;
    };

}

#endif //INKFUSE_COLUMNSCAN_H
