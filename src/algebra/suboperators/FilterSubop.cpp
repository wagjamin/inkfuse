#include "algebra/suboperators/FilterSubop.h"
#include "algebra/CompilationContext.h"
#include "codegen/Expression.h"

namespace inkfuse {

FilterSubop::FilterSubop(const RelAlgOp* source_, std::vector<const IU*> provided_ius_, const IU& filter_iu_)
   : TemplatedSuboperator<EmptyState, EmptyState>(source_, provided_ius_, std::move(provided_ius_)), filter_iu(filter_iu_)
{
}

std::pair<Suboperator::ScopingBehaviour, const IU *> FilterSubop::scopingBehaviour() const
{
   return {ScopingBehaviour::RescopeRetain, &filter_iu};
}

std::string FilterSubop::id() const
{
   // Note that this id is never used during vectorized interpretation - filters don't have to be interpreted.
   // They only rescope the pipeline fow downstream vectorized operators.
   return "FilterSubop";
}

void FilterSubop::consumeAllChildren(CompilationContext& context)
{
   auto& builder = context.getFctBuilder();
   const auto& program = context.getProgram();

   // Resolve incoming operator scope. This is the one where we get source IUs from.
   const auto& decl = context.getIUDeclaration(*this, filter_iu);
   auto expr = IR::VarRefExpr::build(decl);

   // Construct the if - will later be closed in close().
   // All downstream operators will generate their code in the if block.
   opt_if = builder.buildIf(std::move(expr));
   {
      // And notify downstream operators that filtered IUs are now ready.
      context.notifyIUsReady(*this);
   }
}

void FilterSubop::close(CompilationContext& context)
{
   // We can now close the sub-operator, this will terminate the if statement
   // and reinstall the original block.
   opt_if->End();
   opt_if.reset();
   context.notifyOpClosed(*this);
}

}