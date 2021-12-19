// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_ALGEBRA_PRINT_H_
#define INCLUDE_IMLAB_ALGEBRA_PRINT_H_
// ---------------------------------------------------------------------------
#include <memory>
#include <utility>
#include <vector>
#include "imlab/algebra/operator.h"
// ---------------------------------------------------------------------------
namespace imlab {
// ---------------------------------------------------------------------------
class Print: public Operator {
 protected:
    // Child operator
    std::unique_ptr<Operator> child;

    // Required ius
    std::set<const IU*> required_ius;
    // Consumer
    Operator *consumer;

 public:
    // Constructor
    explicit Print(CodegenHelper& codegen_helper_, std::unique_ptr<Operator> child_, std::set<const IU*> required_ius_)
        : Operator(codegen_helper_), child(std::move(child_)), required_ius(std::move(required_ius_)) {}

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
#endif  // INCLUDE_IMLAB_ALGEBRA_PRINT_H_
// ---------------------------------------------------------------------------

