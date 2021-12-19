// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_ALGEBRA_SELECTION_H_
#define INCLUDE_IMLAB_ALGEBRA_SELECTION_H_
// ---------------------------------------------------------------------------
#include <memory>
#include <utility>
#include <vector>
#include <string>
#include "imlab/infra/types.h"
#include "imlab/algebra/operator.h"
#include "imlab/algebra/expression.h"
// ---------------------------------------------------------------------------
namespace imlab {
// ---------------------------------------------------------------------------
class Selection: public Operator {
 protected:
    // Child operator
    std::unique_ptr<Operator> child;
    // Predicate.
    std::unique_ptr<Expression> predicate;

    // Required ius
    std::vector<const IU*> required_ius;
    // Consumer
    Operator *consumer;

 public:
    // Constructor
    Selection(CodegenHelper& codegen_helper_, std::unique_ptr<Operator> child_, std::unique_ptr<Expression> predicate_)
        : Operator(codegen_helper_), child(std::move(child_)), predicate(std::move(predicate_)) {}

    // Collect all IUs produced by the operator
    std::vector<const IU*> CollectIUs() override;

    // Prepare the operator
    void Prepare(const std::set<const IU*> &required, Operator* consumer) override;
    // Produce all tuples
    void Produce() override;
    // Consume tuple
    void Consume(const Operator* child) override;
};
// ---------------------------------------------------------------------------
}  // namespace imlab
// ---------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_ALGEBRA_SELECTION_H_
// ---------------------------------------------------------------------------

