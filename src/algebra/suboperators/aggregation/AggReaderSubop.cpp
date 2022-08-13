#include "algebra/suboperators/aggregation/AggReaderSubop.h"

namespace inkfuse {

SuboperatorArc AggReaderSubop::build(const RelAlgOp* source_, const IU& packed_ptr_iu, const IU& agg_iu, const AggCompute& agg_compute_) {
   return SuboperatorArc(new AggReaderSubop(source_, packed_ptr_iu, agg_iu, agg_compute_));
}

AggReaderSubop::AggReaderSubop(const RelAlgOp* source_, const IU& packed_ptr_iu, const IU& agg_iu, const AggCompute& agg_compute_)
   : TemplatedSuboperator<KeyPackingRuntimeState>(source_, {&agg_iu}, {&packed_ptr_iu}), agg_compute(agg_compute_) {
}

void AggReaderSubop::setUpStateImpl(const ExecutionContext& context) {
   // Runtime params have to be set up during execution and have the right type.
   if (!runtime_params.offset || runtime_params.offset->getType()->id() != "UI2") {
      throw std::runtime_error("RuntimeParam of AggregatorSubop must be set up and of type u16 during execution.");
   }
   // Fetch the underlying raw data from the associated runtime parameters.
   // If the value was hard-coded in the generated code already it will simply never be accessed.
   state->offset = *reinterpret_cast<uint16_t*>(runtime_params.offset->rawData());
}

void AggReaderSubop::consumeAllChildren(CompilationContext& context) {
   auto& builder = context.getFctBuilder();
   auto& packed_ptr = context.getIUDeclaration(*source_ius[0]);
   auto var_identifier = getVarIdentifier().str();

   auto& out_iu = *provided_ius[0];
   auto iu_name = context.buildIUIdentifier(out_iu);
   auto& out_declare = builder.appendStmt(IR::DeclareStmt::build(iu_name, out_iu.type));
   context.declareIU(out_iu, out_declare);

   // TODO Support multi-state aggregates.
   // Compute the pointer to the aggregate state.
   auto ptr_loc = IR::ArithmeticExpr::build(
      IR::VarRefExpr::build(packed_ptr),
      runtime_params.offsetResolve(*this, context),
      IR::ArithmeticExpr::Opcode::Add);
   auto& ptr_declare = builder.appendStmt(IR::DeclareStmt::build(var_identifier + "_packed_state", ptr_loc->type));
   builder.appendStmt(IR::AssignmentStmt::build(ptr_declare, std::move(ptr_loc)));

   // Compute the aggregate function result.
   auto unpack = agg_compute.compute(builder, {&ptr_declare}, out_declare);
   builder.appendStmt(std::move(unpack));

   context.notifyIUsReady(*this);
}

std::string AggReaderSubop::id() const {
   return agg_compute.id();
}

}