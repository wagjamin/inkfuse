#include "algebra/suboperators/row_layout/KeyUnpackerSubop.h"

namespace inkfuse {

SuboperatorArc KeyUnpackerSubop::build(const RelAlgOp* source_, const IU& compound_key_, const IU& target_iu_) {
   return std::shared_ptr<KeyUnpackerSubop>(new KeyUnpackerSubop{source_, compound_key_, target_iu_});
}

KeyUnpackerSubop::KeyUnpackerSubop(const RelAlgOp* source_, const IU& compound_key_, const IU& target_iu_)
   : TemplatedSuboperator<KeyPackingRuntimeState>(source_, {&target_iu_}, {&compound_key_}) {
   auto type = dynamic_cast<IR::Pointer*>(compound_key_.type.get());
   if (!type || !dynamic_cast<IR::Char*>(type->pointed_to.get())) {
      throw std::runtime_error("KeyUnpackerSubop has to read from Ptr<Char>.");
   }
}

void KeyUnpackerSubop::setUpStateImpl(const ExecutionContext& context) {
   // Runtime params have to be set up during execution and have the right type.
   if (!runtime_params.offset || runtime_params.offset->getType()->id() != "UI2") {
      throw std::runtime_error("RuntimeParam of RuntimeKeyExpressionsubop must be set up and of type u16 during execution.");
   }
   // Fetch the underlying raw data from the associated runtime parameters.
   // If the value was hard-coded in the generated code already it will simply never be accessed.
   state->offset = *reinterpret_cast<uint16_t*>(runtime_params.offset->rawData());
}

void KeyUnpackerSubop::consumeAllChildren(CompilationContext& context) {
   auto& builder = context.getFctBuilder();
   const auto& program = context.getProgram();

   // Declare output statement.
   auto iu_name = context.buildIUIdentifier(*provided_ius[0]);
   auto& declare = builder.appendStmt(IR::DeclareStmt::build(iu_name, provided_ius[0]->type));
   context.declareIU(*provided_ius[0], declare);

   const IR::Stmt& ptr = context.getIUDeclaration(*source_ius[0]);

   // Compute pointer location in the packed key.
   auto ptr_loc = IR::ArithmeticExpr::build(
      IR::VarRefExpr::build(ptr),
      runtime_params.offsetResolve(*this, context),
      IR::ArithmeticExpr::Opcode::Add);
   // Cast to the target type and prepare for assignment.
   auto ptr_derefed = IR::DerefExpr::build(IR::CastExpr::build(std::move(ptr_loc), IR::Pointer::build(provided_ius[0]->type)));

   // And write from the pointer offset to the provided IU.
   builder.appendStmt(IR::AssignmentStmt::build(declare, std::move(ptr_derefed)));

   // We don't produce IUs, but we still have to notify the context that we are ready.
   context.notifyIUsReady(*this);
}

std::string KeyUnpackerSubop::id() const {
   std::stringstream res;
   res << "key_unpacker_" << provided_ius[0]->type->id();
   return res.str();
}

}