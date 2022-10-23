#include "algebra/suboperators/aggregation/AggReaderSubop.h"

namespace inkfuse {

SuboperatorArc AggReaderSubop::build(const RelAlgOp* source_, const IU& packed_ptr_iu, const IU& agg_iu, const AggCompute& agg_compute_) {
   return SuboperatorArc(new AggReaderSubop(source_, packed_ptr_iu, agg_iu, agg_compute_));
}

AggReaderSubop::AggReaderSubop(const RelAlgOp* source_, const IU& packed_ptr_iu, const IU& agg_iu, const AggCompute& agg_compute_)
   : TemplatedSuboperator<KeyPackingRuntimeStateTwo>(source_, {&agg_iu}, {&packed_ptr_iu}), agg_compute(agg_compute_) {
   assert(agg_compute.requiredGranules() == 1 || agg_compute.requiredGranules() == 2);
}

void AggReaderSubop::setUpStateImpl(const ExecutionContext& context) {
   // Runtime params have to be set up during execution and have the right type.
   if (!runtime_params.offset_1 || runtime_params.offset_1->getType()->id() != "UI2") {
      throw std::runtime_error("RuntimeParam offset_1 of AggregatorSubop must be set up and of type u16 during execution.");
   }
   if (agg_compute.requiredGranules() > 1 && (!runtime_params.offset_2 || runtime_params.offset_2->getType()->id() != "UI2")) {
      throw std::runtime_error("RuntimeParam offset_2 of AggregatorSubop must be set up and of type u16 during execution.");
   }
   // Fetch the underlying raw data from the associated runtime parameters.
   // If the value was hard-coded in the generated code already it will simply never be accessed.
   state->offset_1 = *reinterpret_cast<uint16_t*>(runtime_params.offset_1->rawData());
   if (agg_compute.requiredGranules() > 1) {
      state->offset_2 = *reinterpret_cast<uint16_t*>(runtime_params.offset_2->rawData());
   }
}

void AggReaderSubop::consumeAllChildren(CompilationContext& context) {
   auto& builder = context.getFctBuilder();
   auto& packed_ptr = context.getIUDeclaration(*source_ius[0]);
   auto var_identifier = getVarIdentifier().str();

   auto& out_iu = *provided_ius[0];
   auto iu_name = context.buildIUIdentifier(out_iu);
   auto& out_declare = builder.appendStmt(IR::DeclareStmt::build(iu_name, out_iu.type));
   context.declareIU(out_iu, out_declare);

   std::vector<IR::Stmt*> granule_declares;
   {
      // Compute the pointer to the first granule in the aggregate state.
      // Always needed.
      auto ptr_loc = IR::ArithmeticExpr::build(
         IR::VarRefExpr::build(packed_ptr),
         runtime_params.offset_1Resolve(*this, context),
         IR::ArithmeticExpr::Opcode::Add);
      auto& ptr_declare = builder.appendStmt(IR::DeclareStmt::build(var_identifier + "_packed_state_1", ptr_loc->type));
      builder.appendStmt(IR::AssignmentStmt::build(ptr_declare, std::move(ptr_loc)));
      granule_declares.push_back(&ptr_declare);
   }

   if (agg_compute.requiredGranules() > 1) {
      // This aggregate function needs a second granule - add it to the runtime state.
      auto ptr_loc = IR::ArithmeticExpr::build(
         IR::VarRefExpr::build(packed_ptr),
         runtime_params.offset_2Resolve(*this, context),
         IR::ArithmeticExpr::Opcode::Add);
      auto& ptr_declare = builder.appendStmt(IR::DeclareStmt::build(var_identifier + "_packed_state_2", ptr_loc->type));
      builder.appendStmt(IR::AssignmentStmt::build(ptr_declare, std::move(ptr_loc)));
      granule_declares.push_back(&ptr_declare);
   }

   // Compute the aggregate function result.
   auto unpack = agg_compute.compute(builder, granule_declares, out_declare);
   builder.appendStmt(std::move(unpack));

   context.notifyIUsReady(*this);
}

std::string AggReaderSubop::id() const {
   return agg_compute.id();
}

}