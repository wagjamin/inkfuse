#include "algebra/suboperators/aggregation/AggregatorSubop.h"
#include "algebra/suboperators/aggregation/AggStateCount.h"

namespace inkfuse {

SuboperatorArc AggregatorSubop::build(const RelAlgOp* source_, const AggState& agg_state_, const IU& ptr_iu, const IU& not_init, const IU& agg_iu) {
   return SuboperatorArc{new AggregatorSubop(source_, agg_state_, ptr_iu, not_init, agg_iu)};
}

AggregatorSubop::AggregatorSubop(const RelAlgOp* source_, const AggState& agg_state_, const IU& ptr_iu, const IU& not_init, const IU& agg_iu)
   : TemplatedSuboperator<KeyPackingRuntimeState>(source_, {}, {&ptr_iu, &not_init, &agg_iu}), agg_state(agg_state_) {
}

std::string AggregatorSubop::id() const {
   return agg_state.id();
}

void AggregatorSubop::setUpStateImpl(const ExecutionContext& context)
{
   // Runtime params have to be set up during execution and have the right type.
   if (!runtime_params.offset || runtime_params.offset->getType()->id() != "UI2") {
      throw std::runtime_error("RuntimeParam of AggregatorSubop must be set up and of type u16 during execution.");
   }
   // Fetch the underlying raw data from the associated runtime parameters.
   // If the value was hard-coded in the generated code already it will simply never be accessed.
   state->offset = *reinterpret_cast<uint16_t*>(runtime_params.offset->rawData());
}

void AggregatorSubop::consumeAllChildren(CompilationContext& context) {
   auto& builder = context.getFctBuilder();
   auto& packed_ptr = context.getIUDeclaration(*source_ius[0]);
   auto& not_init_stmt = context.getIUDeclaration(*source_ius[1]);
   auto& agg_iu_stmt = context.getIUDeclaration(*source_ius[2]);
   auto var_identifier = getVarIdentifier().str();

   // Compute the pointer to the aggregate state.
   auto ptr_loc = IR::ArithmeticExpr::build(
      IR::VarRefExpr::build(packed_ptr),
      runtime_params.offsetResolve(*this, context),
      IR::ArithmeticExpr::Opcode::Add);
   auto& ptr_declare = builder.appendStmt(IR::DeclareStmt::build(var_identifier + "_packed_state", ptr_loc->type));
   builder.appendStmt(IR::AssignmentStmt::build(ptr_declare, std::move(ptr_loc)));

   // Is this the initial time we write to this group?
   IR::If should_init = builder.buildIf(IR::VarRefExpr::build(not_init_stmt));
   {
      // Call the group initialization logic.
      agg_state.initState(builder, ptr_declare, agg_iu_stmt);
   }
   should_init.Else();
   {
      // Call the group update logic.
      agg_state.updateState(builder, ptr_declare, agg_iu_stmt);
   }

}

}
