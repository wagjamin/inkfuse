#include "imlab/algebra/expression.h"
#include <sstream>
// ---------------------------------------------------------------------------
namespace imlab {
// ---------------------------------------------------------------------------
const IU& Expression::getProducedIU() const
{
    if (!produced) {
        throw std::runtime_error("no assigned IU");
    }
    return *produced;
}
// ---------------------------------------------------------------------------
Expression::Expression(std::vector<std::unique_ptr<Expression>> children_): children(std::move(children_))
{
    std::stringstream ptrstr;
    // Set up IU name.
    ptrstr << this;
    name = ptrstr.str();
}
// ---------------------------------------------------------------------------
schemac::Type Expression::getType() const
{
    return getProducedIU().type;
}
// ---------------------------------------------------------------------------
void Expression::evaluate(CodegenHelper &helper) const
{
    for (const auto& child: children) {
        child->evaluate(helper);
    }
    evaluateImpl(helper);
}
// ---------------------------------------------------------------------------
void Expression::getRequiredIUs(std::set<const IU *>& required) const
{
    for (const auto& child: children) {
        child->getRequiredIUs(required);
    }
    getRequiredIUsImpl(required);
}
// ---------------------------------------------------------------------------
UnaryExpression::UnaryExpression(std::unique_ptr<Expression> child_)
    : Expression({})
{
    children.push_back(std::move(child_));
}
// ---------------------------------------------------------------------------
BinaryExpression::BinaryExpression(std::unique_ptr<Expression> child_l_,
                                   std::unique_ptr<Expression> child_r_)
                                   : Expression({})
{
    children.push_back(std::move(child_l_));
    children.push_back(std::move(child_r_));
}
// ---------------------------------------------------------------------------
EqualsExpression::EqualsExpression(std::unique_ptr<Expression> child_l_, std::unique_ptr<Expression> child_r_)
    : BinaryExpression(std::move(child_l_), std::move(child_r_))
{
    if (!(children[0]->getType() == children[1]->getType())) {
        throw std::runtime_error("Types in eq expression do not match");
    }
    produced.emplace(name.c_str(), schemac::Type::Bool());
}
// ---------------------------------------------------------------------------
void EqualsExpression::evaluateImpl(CodegenHelper &helper) const
{
    auto name_l = children[0]->getProducedIU().Varname();
    auto name_r = children[1]->getProducedIU().Varname();
    auto stmt = helper.Stmt();
    stmt << produced->type.Desc() << " " << produced->Varname() << " = (" << name_l << " == " << name_r << ")";
}
// ---------------------------------------------------------------------------
AndExpression::AndExpression(std::unique_ptr<Expression> child_l_, std::unique_ptr<Expression> child_r_)
    : BinaryExpression(std::move(child_l_), std::move(child_r_))
{
    if (children[0]->getType().tclass != schemac::Type::kBool || children[1]->getType().tclass != schemac::Type::kBool) {
        throw std::runtime_error("Types in and expression not boolean");
    }
    produced.emplace(name.c_str(), schemac::Type::Bool());
}
// ---------------------------------------------------------------------------
void AndExpression::evaluateImpl(CodegenHelper &helper) const
{
    auto name_l = children[0]->getProducedIU().Varname();
    auto name_r = children[1]->getProducedIU().Varname();
    auto stmt = helper.Stmt();
    stmt << produced->type.Desc() << " " << produced->Varname() << " = (" << name_l << " && " << name_r << ")";
}
// ---------------------------------------------------------------------------
ConstantExpression::ConstantExpression(schemac::Type type, std::string desc_): Expression({}), desc(std::move(desc_))
{
    produced.emplace(desc.c_str(), type);
}
// ---------------------------------------------------------------------------
void ConstantExpression::evaluateImpl(CodegenHelper &helper) const
{
    auto stmt = helper.Stmt();
    stmt << produced->type.Desc() << " " << produced->Varname();
    stmt << " = " << desc;
}
// ---------------------------------------------------------------------------
IURefExpression::IURefExpression(const IU& iu_): Expression({}), iu(iu_)
{
}
// ---------------------------------------------------------------------------
void IURefExpression::evaluateImpl(CodegenHelper &helper) const
{
    // NOOP, IU exists already.
}
// ---------------------------------------------------------------------------
void IURefExpression::getRequiredIUsImpl(std::set<const IU *>& required) const
{
    // Add base IU going into this expression.
    required.emplace(&iu);
}
// ---------------------------------------------------------------------------
const IU& IURefExpression::getProducedIU() const
{
    return iu;
}
// ---------------------------------------------------------------------------
} // namespace imlab
// ---------------------------------------------------------------------------
