// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_ALGEBRA_OPERATOR_H_
#define INCLUDE_IMLAB_ALGEBRA_OPERATOR_H_
// ---------------------------------------------------------------------------
#include <vector>
#include <set>
#include "imlab/algebra/iu.h"
#include "imlab/algebra/codegen_helper.h"
// ---------------------------------------------------------------------------
namespace imlab {
// ---------------------------------------------------------------------------
class Operator {
 protected:
    // The helper for code generation
    CodegenHelper& codegen_helper;

 public:
    // Constructor
    explicit Operator(CodegenHelper& codegen_helper_) : codegen_helper(codegen_helper_) {}

    virtual ~Operator() = default;

    // Collect all IUs produced by the operator
    virtual std::vector<const IU*> CollectIUs() = 0;

    CodegenHelper& getHelper() { return codegen_helper; }

    // Prepare the operator
    virtual void Prepare(const std::set<const IU*> &required, Operator* parent) = 0;
    // Produce all tuples
    virtual void Produce() = 0;
    // Consume tuple
    virtual void Consume(const Operator* child) = 0;
};
// ---------------------------------------------------------------------------
using OperatorPtr = std::unique_ptr<Operator>;
// ---------------------------------------------------------------------------
}  // namespace imlab
// ---------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_ALGEBRA_OPERATOR_H_
// ---------------------------------------------------------------------------

