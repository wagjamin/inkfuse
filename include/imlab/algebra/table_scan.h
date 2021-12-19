// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_ALGEBRA_TABLE_SCAN_H_
#define INCLUDE_IMLAB_ALGEBRA_TABLE_SCAN_H_
// ---------------------------------------------------------------------------
#include <memory>
#include <utility>
#include <vector>
#include "imlab/algebra/operator.h"
#include "imlab/storage/relation.h"
// ---------------------------------------------------------------------------
namespace imlab {
// ---------------------------------------------------------------------------
class TableScan: public Operator {
 protected:
    // Required ius
    std::set<const IU*> required_ius;
    // Consumer
    Operator *consumer;
    // IUs produced by this scan. Maps from column name to IU.
    // Note: pointer stability has to be guaranteed after CollectIUs was called.
    std::vector<IU> produced_ius;
    // Backing relation.
    const StoredRelation& rel;
    // Backing relation name.
    const char* relname;

 public:
    // Constructor
    TableScan(CodegenHelper& codegen_helper_, const StoredRelation& rel_, const char* relname_) : Operator(codegen_helper_), rel(rel_), relname(relname_) {}

    // Collect all IUs produced by the operator
    std::vector<const IU*> CollectIUs() override;

    // Prepare the operator
    void Prepare(const std::set<const IU*> &required, Operator* consumer_) override;

    // Produce all tuples
    void Produce() override;
    // Consume tuple
    void Consume(const Operator* child) override {}
};
// ---------------------------------------------------------------------------
}  // namespace imlab
// ---------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_ALGEBRA_TABLE_SCAN_H_
// ---------------------------------------------------------------------------

