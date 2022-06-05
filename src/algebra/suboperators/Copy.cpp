#include "algebra/suboperators/Copy.h"
#include "algebra/CompilationContext.h"

namespace inkfuse {

std::unique_ptr<Copy> Copy::build(const IU& copy_iu)
{
   return std::unique_ptr<Copy>(new Copy(copy_iu));
}

void Copy::consume(const IU& iu, CompilationContext& context)
{
   auto& builder = context.getFctBuilder();

   const auto& in = context.getIUDeclaration(*this, iu);
   // Declare the new IU.
   auto declare = IR::DeclareStmt::build(context.buildIUIdentifier(*this, produced), iu.type);
   auto assign = IR::AssignmentStmt::build(*declare, IR::VarRefExpr::build(in));

   // Register it with the context.
   context.declareIU(*this, produced, *declare);

   // Add the statements to the generated program.
   builder.appendStmt(std::move(declare));
   builder.appendStmt(std::move(assign));

   // And notify consumer that IUs are ready.
   context.notifyIUsReady(*this);
}

std::string Copy::id() const
{
   return "Copy_" + produced.type->id();
}

Copy::Copy(const IU& copy_iu)
: TemplatedSuboperator<EmptyState, EmptyState>(nullptr, {&produced}, {&copy_iu}), produced(copy_iu.type)
{
}

}
