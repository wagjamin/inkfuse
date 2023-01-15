#include "algebra/suboperators/row_layout/KeyPackerSubop.h"
#include <sstream>

namespace inkfuse {

IR::StmtPtr KeyPacking::packInto(IR::ExprPtr ptr, IR::ExprPtr to_pack, IR::ExprPtr offset) {
   // Compute pointer location into which to pack.
   auto ptr_loc = IR::ArithmeticExpr::build(
      std::move(ptr), std::move(offset),
      IR::ArithmeticExpr::Opcode::Add);
   // Cast to the target type and prepare for assignment.
   auto ptr_derefed = IR::DerefExpr::build(IR::CastExpr::build(std::move(ptr_loc), IR::Pointer::build(to_pack->type)));
   // And assign.
   return IR::AssignmentStmt::build(std::move(ptr_derefed), std::move(to_pack));
}

SuboperatorArc KeyPackerSubop::build(const RelAlgOp* source_, const IU& to_pack_, const IU& compound_key_, std::vector<const IU*> pseudo_ius) {
   return std::shared_ptr<KeyPackerSubop>(new KeyPackerSubop{source_, to_pack_, compound_key_, std::move(pseudo_ius)});
}

KeyPackerSubop::KeyPackerSubop(const RelAlgOp* source_, const IU& to_pack_, const IU& compound_key_, std::vector<const IU*> pseudo_ius)
   : TemplatedSuboperator<KeyPackingRuntimeState>(source_, pseudo_ius, {&to_pack_, &compound_key_}) {
   auto is_ptr = dynamic_cast<IR::Pointer*>(compound_key_.type.get());
   if (!is_ptr || !dynamic_cast<IR::Char*>(is_ptr->pointed_to.get())) {
      auto is_byte_array = dynamic_cast<IR::ByteArray*>(compound_key_.type.get());
      if (!is_byte_array) {
         throw std::runtime_error("KeyPackerSubop has to write into Ptr<Char> or ByteArray.");
      }
   }
}

void KeyPackerSubop::setUpStateImpl(const ExecutionContext& context) {
   // Runtime params have to be set up during execution and have the right type.
   if (!runtime_params.offset || runtime_params.offset->getType()->id() != "UI2") {
      throw std::runtime_error("RuntimeParam of RuntimeKeyExpressionsubop must be set up and of type u16 during execution.");
   }
   // Fetch the underlying raw data from the associated runtime parameters.
   // If the value was hard-coded in the generated code already it will simply never be accessed.
   state->offset = *reinterpret_cast<uint16_t*>(runtime_params.offset->rawData());
}

void KeyPackerSubop::consumeAllChildren(CompilationContext& context) {
   auto& builder = context.getFctBuilder();
   const auto& program = context.getProgram();

   const IR::Stmt& ptr = context.getIUDeclaration(*source_ius[1]);
   const IR::Stmt& pack = context.getIUDeclaration(*source_ius[0]);

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

   // Pack the key.
   auto pack_stmt = KeyPacking::packInto(IR::VarRefExpr::build(ptr), IR::VarRefExpr::build(pack), IR::VarRefExpr::build(*rt_o_declare));
   builder.appendStmt(std::move(pack_stmt));

   // There might be pseudo-IUs that we now have to mark as ready.
   if (!provided_ius.empty()) {
      context.notifyIUsReady(*this);
   }
}

std::string KeyPackerSubop::id() const {
   std::stringstream res;
   res << "key_packer";
   for (const IU* iu : source_ius) {
      res << "_" << iu->type->id();
   }
   return res.str();
}

}