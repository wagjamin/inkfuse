#ifndef INKFUSE_COLUMNSCAN_H
#define INKFUSE_COLUMNSCAN_H

#include "codegen/Types.h"
#include "algebra/Suboperator.h"
#include "codegen/Expression.h"
#include "storage/Relation.h"

namespace inkfuse {

    /// State needed to get a table scan to operate on underlying data. Provided at runtime.
    struct ColumnScanState {
        /// Raw pointer to the column start.
        const void* data;
        /// Backing table size.
        const uint64_t size;
    };

    /// Parameters needed to generate code for a column scan.
    struct ColumnScanParameters {
        /// Type being read by the table scan.
        IR::TypePtr type;
        /// ID of this operator. Needed to access ColumnScanState at runtime.
        IR::SubstitutableParameter<IR::UnsignedInt> operator_id;
    };

    struct ColumnScan : public Suboperator {
    public:
        /// Set up a new column scan.
        ColumnScan(ColumnScanParameters params, IURef provided_iu_, BaseColumn& column_);

        void compile(CompilationContext& context, const std::set<IURef>& required) override;

        void interpret(FuseChunk* src, FuseChunk* dst) override;

        void setUpState() override;

        void tearDownState() override;

    private:
        ColumnScanParameters params;
        /// IU provided by this scan.
        IURef provided_iu;
        /// Backing column to read from.
        const BaseColumn& column;
        /// Backing state.
        std::unique_ptr<ColumnScanState> state;
    };

}

#endif //INKFUSE_COLUMNSCAN_H
