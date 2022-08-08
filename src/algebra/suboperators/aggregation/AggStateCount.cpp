#include "algebra/suboperators/aggregation/AggStateCount.h"
#include "algebra/CompilationContext.h"
#include "codegen/Statement.h"

namespace inkfuse {

AggStateCount::AggStateCount()
 : AggState(std::move(IR::Void::build()))
{
}

void AggStateCount::initState(IR::FunctionBuilder& builder, IR::ExprPtr ptr, IR::ExprPtr) const
{
   // Initial count after the first row was aggregated is always one.
   builder.appendStmt(IR::AssignmentStmt::build(IR::DerefExpr::build(std::move(ptr)), IR::ConstExpr::build(IR::UI<8>::build(1))));
}

void AggStateCount::updateState(IR::FunctionBuilder& builder, IR::ExprPtr ptr, IR::ExprPtr) const
{
    // Increment count by one.
    builder.appendStmt(IR::AssignmentStmt::build(IR::DerefExpr::build(std::move(ptr)), IR::ConstExpr::build(IR::UI<8>::build(1))));
}

size_t AggStateCount::getStateSize() const
{
   return 8;
}

std::string AggStateCount::id() const
{
   return "agg_state_count";
}

}