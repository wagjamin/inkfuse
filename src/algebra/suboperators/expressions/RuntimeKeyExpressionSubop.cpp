#include "algebra/suboperators/expressions/RuntimeKeyExpressionSubop.h"
#include "codegen/IRBuilder.h"

namespace inkfuse {

RuntimeKeyExpressionSubop::RuntimeKeyExpressionSubop(const RelAlgOp* source_, const IU& provided_iu_, const IU& source_iu_elem, const IU& source_iu_ptr)
   : TemplatedSuboperator<KeyPackingRuntimeState>(source_, {&provided_iu_}, {&source_iu_elem, &source_iu_ptr}) {
   if (!dynamic_cast<IR::Bool*>(provided_iu_.type.get())) {
      throw std::runtime_error("RuntimeKeyExpression must provide a boolean output IU.");
   }
   auto type = dynamic_cast<IR::Pointer*>(source_iu_ptr.type.get());
   if (!type || !dynamic_cast<IR::Char*>(type->pointed_to.get())) {
      throw std::runtime_error("RuntimeKeyExpressionSbuop has to check equality on Ptr<Char>.");
   }
}

SuboperatorArc RuntimeKeyExpressionSubop::build(const RelAlgOp* source_, const IU& provided_iu_, const IU& source_iu_elem, const IU& source_iu_ptr) {
   return std::shared_ptr<RuntimeKeyExpressionSubop>(new RuntimeKeyExpressionSubop(source_, provided_iu_, source_iu_elem, source_iu_ptr));
}

void RuntimeKeyExpressionSubop::setUpStateImpl(const ExecutionContext& context) {
   // Runtime params have to be set up during execution and have the right type.
   if (!runtime_params.offset || runtime_params.offset->getType()->id() != "UI2") {
      throw std::runtime_error("RuntimeParam of RuntimeKeyExpressionsubop must be set up and of type u16 during execution.");
   }
   // Fetch the underlying raw data from the associated runtime parameters.
   // If the value was hard-coded in the generated code already it will simply never be accessed.
   state->offset = *reinterpret_cast<uint16_t*>(runtime_params.offset->rawData());
}

void RuntimeKeyExpressionSubop::consumeAllChildren(CompilationContext& context) {
   // All children were produced, we simply have to generate code for this specific expression.
   auto& builder = context.getFctBuilder();
   const auto& program = context.getProgram();
   const IU* out = *provided_ius.begin();

   // First, declare the output IU.
   auto iu_name = context.buildIUIdentifier(*out);
   auto& declare = builder.appendStmt(IR::DeclareStmt::build(iu_name, out->type));
   context.declareIU(*out, declare);

   // Then, compute it. Lazily resolve the offset argument.
   const IR::Stmt& ptr = context.getIUDeclaration(*source_ius[1]);
   const IR::Stmt& source = context.getIUDeclaration(*source_ius[0]);
   // Compute pointer location into which to pack.
   auto ptr_loc = IR::ArithmeticExpr::build(
      IR::VarRefExpr::build(ptr),
      runtime_params.offsetResolve(*this, context),
      IR::ArithmeticExpr::Opcode::Add);
   // Cast to pointer of target type and deref for comparison.
   auto ptr_derefed = IR::DerefExpr::build(IR::CastExpr::build(std::move(ptr_loc), IR::Pointer::build(source_ius[0]->type)));
   // Access raw data.
   auto compare = IR::VarRefExpr::build(source);
   // And compare them.
   builder.appendStmt(
      IR::AssignmentStmt::build(
         declare,
         IR::ArithmeticExpr::build(
            std::move(ptr_derefed),
            std::move(compare),
            IR::ArithmeticExpr::Opcode::Eq)));

   context.notifyIUsReady(*this);
}

std::string RuntimeKeyExpressionSubop::id() const {
   std::stringstream res;
   res << "runtime_key_expr_eq_";
   for (auto child : source_ius) {
      res << "_" << child->type->id();
   }
   return res.str();
}

}
