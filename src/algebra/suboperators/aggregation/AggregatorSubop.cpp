#include "algebra/suboperators/aggregation/AggregatorSubop.h"
#include "algebra/suboperators/aggregation/AggStateCount.h"

namespace inkfuse {

SuboperatorArc AggregatorSubop::build(const RelAlgOp* source_, const AggState& agg_state_, const IU& ptr_iu, const IU& agg_iu) {
   return SuboperatorArc{new AggregatorSubop(source_, agg_state_, ptr_iu, agg_iu)};
}

AggregatorSubop::AggregatorSubop(const RelAlgOp* source_, const AggState& agg_state_, const IU& ptr_iu, const IU& agg_iu)
   : TemplatedSuboperator<KeyPackingRuntimeState>(source_, {}, {&ptr_iu, &agg_iu}), agg_state(agg_state_) {
}

std::string AggregatorSubop::id() const {
   return agg_state.id();
}

void AggregatorSubop::setUpStateImpl(const ExecutionContext& context) {
   // Runtime params have to be set up during execution and have the right type.
   if (!runtime_params.offset || runtime_params.offset->getType()->id() != "UI2") {
      throw std::runtime_error("RuntimeParam of AggregatorSubop must be set up and of type u16 during execution.");
   }
   for (auto& state : *states) {
      // Fetch the underlying raw data from the associated runtime parameters.
      // If the value was hard-coded in the generated code already it will simply never be accessed.
      state.offset = *reinterpret_cast<uint16_t*>(runtime_params.offset->rawData());
   }
}

void AggregatorSubop::consumeAllChildren(CompilationContext& context) {
   auto& builder = context.getFctBuilder();
   auto& packed_ptr = context.getIUDeclaration(*source_ius[0]);
   auto& agg_iu_stmt = context.getIUDeclaration(*source_ius[1]);
   auto var_identifier = getVarIdentifier().str();

   // Resolve the offset once in the opening scope of the program.
   IR::Stmt* rt_o_declare;
   {
      auto& root = builder.getRootBlock();
      auto rt_offset_name = getVarIdentifier();
      rt_offset_name << "_rt_offset";
      auto rt_offset_declare = IR::DeclareStmt::build(rt_offset_name.str(), IR::UnsignedInt::build(2));
      auto rt_offset_load = IR::AssignmentStmt::build(IR::VarRefExpr::build(*rt_offset_declare), runtime_params.offsetResolve(*this, context));
      rt_o_declare = rt_offset_declare.get();
      root.appendStmt(std::move(rt_offset_declare));
      root.appendStmt(std::move(rt_offset_load));
   }

   // Compute the pointer to the aggregate state.
   auto ptr_loc = IR::ArithmeticExpr::build(
      IR::VarRefExpr::build(packed_ptr),
      IR::VarRefExpr::build(*rt_o_declare),
      IR::ArithmeticExpr::Opcode::Add);
   auto& ptr_declare = builder.appendStmt(IR::DeclareStmt::build(var_identifier + "_packed_state", ptr_loc->type));
   builder.appendStmt(IR::AssignmentStmt::build(ptr_declare, std::move(ptr_loc)));

   // Call the group update logic.
   agg_state.updateState(builder, ptr_declare, agg_iu_stmt);
}

}
