#include "algebra/suboperators/row_layout/KeyUnpackerSubop.h"

namespace inkfuse {

IR::StmtPtr KeyPacking::unpackFrom(IR::ExprPtr ptr, IR::ExprPtr unpack_into, IR::ExprPtr offset) {
   // Compute pointer location from which to unpack.
   auto ptr_loc = IR::ArithmeticExpr::build(
      std::move(ptr), std::move(offset),
      IR::ArithmeticExpr::Opcode::Add);
   // Cast to the target type and prepare for assignment.
   auto ptr_derefed = IR::DerefExpr::build(IR::CastExpr::build(std::move(ptr_loc), IR::Pointer::build(unpack_into->type)));
   // And write from the pointer offset to the provided IU.
   return IR::AssignmentStmt::build(std::move(unpack_into), std::move(ptr_derefed));
}

SuboperatorArc KeyUnpackerSubop::build(const RelAlgOp* source_, const IU& compound_key_, const IU& target_iu_) {
   return std::shared_ptr<KeyUnpackerSubop>(new KeyUnpackerSubop{source_, compound_key_, target_iu_});
}

KeyUnpackerSubop::KeyUnpackerSubop(const RelAlgOp* source_, const IU& compound_key_, const IU& target_iu_)
   : TemplatedSuboperator<KeyPackingRuntimeState>(source_, {&target_iu_}, {&compound_key_}) {
   auto is_ptr = dynamic_cast<IR::Pointer*>(compound_key_.type.get());
   if (!is_ptr || !dynamic_cast<IR::Char*>(is_ptr->pointed_to.get())) {
      auto is_byte_array = dynamic_cast<IR::ByteArray*>(compound_key_.type.get());
      if (!is_byte_array) {
         throw std::runtime_error("KeyUnpackerSubop has to read from Ptr<Char> or ByteArray.");
      }
   }
}

void KeyUnpackerSubop::setUpStateImpl(const ExecutionContext& context) {
   // Runtime params have to be set up during execution and have the right type.
   if (!runtime_params.offset || runtime_params.offset->getType()->id() != "UI2") {
      throw std::runtime_error("RuntimeParam of RuntimeKeyExpressionsubop must be set up and of type u16 during execution.");
   }
   for (auto& state : *states) {
      // Fetch the underlying raw data from the associated runtime parameters.
      // If the value was hard-coded in the generated code already it will simply never be accessed.
      state.offset = *reinterpret_cast<uint16_t*>(runtime_params.offset->rawData());
   }
}

void KeyUnpackerSubop::consumeAllChildren(CompilationContext& context) {
   auto& builder = context.getFctBuilder();
   const auto& program = context.getProgram();

   // Declare output statement.
   auto iu_name = context.buildIUIdentifier(*provided_ius[0]);
   auto& declare = builder.appendStmt(IR::DeclareStmt::build(iu_name, provided_ius[0]->type));
   context.declareIU(*provided_ius[0], declare);

   const IR::Stmt& ptr = context.getIUDeclaration(*source_ius[0]);

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

   // Unpack the key.
   auto unpack_stmt = KeyPacking::unpackFrom(IR::VarRefExpr::build(ptr), IR::VarRefExpr::build(declare), IR::VarRefExpr::build(*rt_o_declare));
   // And write from the pointer offset to the provided IU.
   builder.appendStmt(std::move(unpack_stmt));

   // We don't produce IUs, but we still have to notify the context that we are ready.
   context.notifyIUsReady(*this);
}

std::string KeyUnpackerSubop::id() const {
   std::stringstream res;
   res << "key_unpacker_";
   res << source_ius[0]->type->id() << "_";
   res << provided_ius[0]->type->id();
   return res.str();
}

}
