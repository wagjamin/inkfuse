#include "algebra/suboperators/sinks/CountingSink.h"
#include "algebra/CompilationContext.h"
#include "runtime/Runtime.h"

namespace inkfuse {

const char* CountingState::name = "CountingState";

// static
SuboperatorArc CountingSink::build(const IU& iu)
{
   return std::make_shared<CountingSink>(iu);
}

CountingSink::CountingSink(const IU& input_iu)
: TemplatedSuboperator<CountingState, EmptyState>(nullptr, {}, {&input_iu})
{
}

void CountingSink::consume(const IU& iu, CompilationContext& context)
{
   assert(&iu == *this->source_ius.begin());
   auto& builder = context.getFctBuilder();
   const auto& program = context.getProgram();

   // Get global state.
   auto state_expr_1 = context.accessGlobalState(*this);
   auto state_expr_2 = context.accessGlobalState(*this);
   // Cast it to a TScanDriverState pointer.
   auto cast_expr_1 = IR::CastExpr::build(std::move(state_expr_1), IR::Pointer::build(program.getStruct(CountingState::name)));
   auto cast_expr_2 = IR::CastExpr::build(std::move(state_expr_2), IR::Pointer::build(program.getStruct(CountingState::name)));

   // And add to the counter.
   auto increment_start = IR::AssignmentStmt::build(
      IR::StructAccessExpr::build(std::move(cast_expr_1), "count"),
      IR::ArithmeticExpr::build(
         IR::StructAccessExpr::build(std::move(cast_expr_2), "count"),
         IR::ConstExpr::build(IR::UI<8>::build(1)),
         IR::ArithmeticExpr::Opcode::Add
      )
   );

   builder.appendStmt(std::move(increment_start));
}

void CountingSink::setUpStateImpl(const ExecutionContext& context)
{
   state->count = 0;
}

std::string CountingSink::id() const
{
   return "CountingSink";
}

void CountingSink::registerRuntime()
{
   RuntimeStructBuilder{CountingState::name}
      .addMember("count", IR::UnsignedInt::build(8));
}

}
