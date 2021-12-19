#include "imlab/algebra/table_scan.h"
#include "imlab/infra/settings.h"
#include "tbb/parallel_for.h"
#include <sstream>
// ---------------------------------------------------------------------------
namespace imlab {
// ---------------------------------------------------------------------------
std::vector<const IU *> TableScan::CollectIUs()
{
    // Populate owned IUs if it wasn't done before.
    if (produced_ius.empty()) {
        for (size_t k = 0; k < rel.columnCount(); ++k) {
            const auto& [colname, col]= rel.getColumn(k);
            produced_ius.emplace_back(relname, colname.data(), col.getBackingType());
        }
    }

    // Return them.
    std::vector<const IU*> result;
    result.reserve(produced_ius.size());
    for (const auto& iu: produced_ius) {
        result.push_back(&iu);
    }
    return result;
}
// ---------------------------------------------------------------------------
void TableScan::Prepare(const std::set<const IU *> &required, Operator *consumer_)
{
    // Get IUs which are produced by this scan.
    auto myIUs = CollectIUs();
    for (auto iu: myIUs) {
        if (required.count(iu)) {
            required_ius.emplace(iu);
        }
    }
    consumer = consumer_;
}
// ---------------------------------------------------------------------------
void TableScan::Produce()
{
    // Prepare IUs and set up loop over rows.
    std::stringstream it_name_stream;
    it_name_stream << "scan_" << this << "_tid";
    std::string it_name = it_name_stream.str();
    std::stringstream loop_stmt_stream;
    // Driving loop around the scan. Depends on whether TBB is enabled.
    if (Settings::USE_TBB) {

        loop_stmt_stream << "tbb::parallel_for(";
        loop_stmt_stream << "tbb::blocked_range<int>(0, db." << relname << ".getSize()), ";
        loop_stmt_stream << "[&](tbb::blocked_range<int> r)";
    } else {
        loop_stmt_stream << "for(";
        loop_stmt_stream << "size_t " << it_name << " = 0; ";
        loop_stmt_stream << it_name << " < db." << relname << ".getSize() ;";
        loop_stmt_stream << it_name << "++)";
    }

    codegen_helper.Flow() << loop_stmt_stream.str();

    {
        auto scope = codegen_helper.CreateScopedBlock();

        // Declare produced IUs.
        for (auto prod_iu: required_ius) {
            codegen_helper.Stmt() << prod_iu->type.Desc() << " " << prod_iu->Varname();
        }

        if constexpr (Settings::USE_TBB) {
            std::stringstream tbb_loop_stream;
            // Actual driver over TBB range received in the task
            tbb_loop_stream << "for(";
            tbb_loop_stream << "size_t " << it_name << "= r.begin(); ";
            tbb_loop_stream << it_name << " < r.end();";
            tbb_loop_stream << it_name << "++)";
            codegen_helper.Flow() << tbb_loop_stream.str();
        }

        {
            auto tbb_scope = codegen_helper.CreateScopedBlock();

            // Read tuple at current index.
            std::stringstream row_stream;
            row_stream << "auto row_" << this << " = ";
            row_stream << "db." << relname << ".Read(" << it_name << ")";
            codegen_helper.Stmt() << row_stream.str();
            // Set up required IUs.
            for (auto prod_iu: required_ius) {
                size_t col_id = rel.getColumnId(prod_iu->column);
                std::stringstream iu_stream;
                iu_stream << prod_iu->Varname();
                iu_stream << " = std::get<" << col_id << ">(row_" << this << ")";
                codegen_helper.Stmt() << iu_stream.str();
            }
            // Consume in parent.
            consumer->Consume(this);
        }
    }

    if (Settings::USE_TBB) {
        // Closing parallel for lambda body.
        codegen_helper.Stmt() << ")";
    }
}
// ---------------------------------------------------------------------------
} // namespace imlab
// ---------------------------------------------------------------------------
