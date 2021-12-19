#include "imlab/algebra/selection.h"
// --------------------------------------------------------------------------
namespace imlab {
// ---------------------------------------------------------------------------
std::vector<const IU *> Selection::CollectIUs()
{
    // Selection simply forwards IUs.
    return child->CollectIUs();
}
// ---------------------------------------------------------------------------
void Selection::Prepare(const std::set<const IU *> &required, Operator *consumer_)
{
    consumer = consumer_;
    std::set<const IU *> required_new = required;
    // Attach IUs required by predicate.
    predicate->getRequiredIUs(required_new);
    // And pass on to child.
    child->Prepare(required_new, this);
}
// ---------------------------------------------------------------------------
void Selection::Produce()
{
    // Simply pass on to child.
    child->Produce();
}
// ---------------------------------------------------------------------------
void Selection::Consume(const Operator *child)
{
    // Evaluate expression.
    predicate->evaluate(codegen_helper);
    // If the predicate is true, pass on to child.
    codegen_helper.Flow() << "if (" << predicate->getProducedIU().Varname() <<  ")";
    {
        // Start a new scoped block.
        auto block = codegen_helper.CreateScopedBlock();
        // And pass on the tuple.
        consumer->Consume(this);
    }
}
// ---------------------------------------------------------------------------
} // namespace imlab
// ---------------------------------------------------------------------------
