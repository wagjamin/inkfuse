#ifndef IMLAB_EXPRESSION_H
#define IMLAB_EXPRESSION_H
// ---------------------------------------------------------------------------
#include "imlab/algebra/codegen_helper.h"
#include "imlab/algebra/iu.h"
#include <optional>
#include <set>
// ---------------------------------------------------------------------------
namespace imlab {
// ---------------------------------------------------------------------------
struct Expression {
public:
    /// Get the IU produced by this expression.
    virtual const IU& getProducedIU() const;

    /// Get the required IUs.
    void getRequiredIUs(std::set<const IU*>& required) const;
    virtual void getRequiredIUsImpl(std::set<const IU*>& required) const {};

    /// Evaluate the expression.
    void evaluate(CodegenHelper& helper) const;
    virtual void evaluateImpl(CodegenHelper& helper) const = 0;


    schemac::Type getType() const;

    /// Virtual Base Destructor.
    virtual ~Expression() = default;

protected:
    using ExpressionVec = std::vector<std::unique_ptr<Expression>>;

    /// Constructor.
    explicit Expression(ExpressionVec children_);

    /// Produced output IU by this expression.
    std::optional<IU> produced;

    /// Unique expression identifier.
    std::string name;

    /// Children.
    ExpressionVec children;
};
// ---------------------------------------------------------------------------
struct UnaryExpression : public Expression {
public:
    explicit UnaryExpression(std::unique_ptr<Expression> child_);
};
// ---------------------------------------------------------------------------
struct BinaryExpression : public Expression {
public:
    BinaryExpression(std::unique_ptr<Expression> child_l_, std::unique_ptr<Expression> child_r_);
};
// ---------------------------------------------------------------------------
struct EqualsExpression : public BinaryExpression {
public:
    EqualsExpression(std::unique_ptr<Expression> child_l_, std::unique_ptr<Expression> child_r_);

protected:
    void evaluateImpl(CodegenHelper& helper) const override;
};
// ---------------------------------------------------------------------------
struct AndExpression : public BinaryExpression {
public:
    AndExpression(std::unique_ptr<Expression> child_l_, std::unique_ptr<Expression> child_r_);

protected:
    void evaluateImpl(CodegenHelper& helper) const override;
};
// ---------------------------------------------------------------------------
struct ConstantExpression final : public Expression {
public:
    ConstantExpression(schemac::Type type, std::string desc_);

    void evaluateImpl(CodegenHelper& helper) const override;

private:
    const std::string desc;
};
// ---------------------------------------------------------------------------
struct IURefExpression final : public Expression {
public:
    explicit IURefExpression(const IU& iu_);

    void evaluateImpl(CodegenHelper& helper) const override;

    void getRequiredIUsImpl(std::set<const IU*>& required) const override;

    const IU& getProducedIU() const override;

private:
    const IU& iu;
};
// ---------------------------------------------------------------------------
using ExpressionPtr = std::unique_ptr<Expression>;
// ---------------------------------------------------------------------------
} // namespace imlab
// ---------------------------------------------------------------------------
#endif //IMLAB_EXPRESSION_H
