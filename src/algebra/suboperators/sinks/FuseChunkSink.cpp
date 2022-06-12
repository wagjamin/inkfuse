#include "algebra/suboperators/sinks/FuseChunkSink.h"
#include "algebra/CompilationContext.h"
#include "exec/ExecutionContext.h"
#include "runtime/Runtime.h"

namespace inkfuse {

const char* FuseChunkSinkState::name = "FuseChunkSinkState";

void FuseChunkSink::registerRuntime() {
   RuntimeStructBuilder(FuseChunkSinkState::name)
      .addMember("raw_data", IR::Pointer::build(IR::Void::build()))
      .addMember("size", IR::Pointer::build(IR::UnsignedInt::build(8)));
}

std::unique_ptr<FuseChunkSink> FuseChunkSink::build(const RelAlgOp* source, const IU& iu_to_write) {
   return std::unique_ptr<FuseChunkSink>(new FuseChunkSink(source, iu_to_write));
}

FuseChunkSink::FuseChunkSink(const RelAlgOp* source, const IU& iu_to_write): Suboperator(source, {}, {&iu_to_write})
{
}

void FuseChunkSink::consume(const IU& iu, CompilationContext& context) {
   assert(&iu == *source_ius.begin());
   auto& builder = context.getFctBuilder();
   const auto& program = context.getProgram();

   const auto& input_iu = context.getIUDeclaration(**source_ius.begin());

   const IR::Stmt* decl_data_ptr;
   const IR::Stmt* decl_size_ptr;
   {
      // In a first step we get the raw data pointer and size pointer and extract it into the root scope.
      auto state_expr_1 = context.accessGlobalState(*this);
      auto state_expr_2 = context.accessGlobalState(*this);
      // Cast it to a FuseChunkSinkState pointer.
      auto data_cast_expr = IR::CastExpr::build(std::move(state_expr_1), IR::Pointer::build(program.getStruct(FuseChunkSinkState::name)));
      auto size_cast_expr = IR::CastExpr::build(std::move(state_expr_2), IR::Pointer::build(program.getStruct(FuseChunkSinkState::name)));
      // Build data variable.
      auto data_var_name = getVarIdentifier();
      data_var_name << "_raw_data";
      auto target_ptr_type = IR::Pointer::build(iu.type);
      auto decl_data = IR::DeclareStmt::build(data_var_name.str(), target_ptr_type);
      decl_data_ptr = decl_data.get();
      // And assign the casted raw pointer.
      auto assign_data = IR::AssignmentStmt::build(
         *decl_data,
         IR::CastExpr::build(
            IR::StructAccesExpr::build(std::move(data_cast_expr), "raw_data"),
            target_ptr_type));
      // Build the size variable.
      auto size_var_name = getVarIdentifier();
      size_var_name << "_size";
      auto size_ptr_type = IR::Pointer::build(IR::UnsignedInt::build(8));
      auto decl_size = IR::DeclareStmt::build(size_var_name.str(), size_ptr_type);
      decl_size_ptr = decl_size.get();
      // And assign the casted raw pointer.
      auto assign_size = IR::AssignmentStmt::build(
         *decl_size,
         IR::StructAccesExpr::build(std::move(size_cast_expr), "size"));
      // Add this to the function preamble.
      std::deque<IR::StmtPtr> preamble_stmts;
      preamble_stmts.push_back(std::move(decl_data));
      preamble_stmts.push_back(std::move(assign_data));
      preamble_stmts.push_back(std::move(decl_size));
      preamble_stmts.push_back(std::move(assign_size));
      builder.getRootBlock().appendStmts(std::move(preamble_stmts));
   }

   // Assign value into output buffer. This is done by adding the offset to the data pointer and dereferencing.
   auto assign = IR::AssignmentStmt::build(
      IR::DerefExpr::build(
         IR::ArithmeticExpr::build(
            IR::VarRefExpr::build(*decl_data_ptr),
            IR::DerefExpr::build(IR::VarRefExpr::build(*decl_size_ptr)),
            IR::ArithmeticExpr::Opcode::Add)),
      IR::VarRefExpr::build(input_iu));

   // And update the size counter.
   auto update_counter = IR::AssignmentStmt::build(
      IR::DerefExpr::build(IR::VarRefExpr::build(*decl_size_ptr)),
      IR::ArithmeticExpr::build(
         IR::DerefExpr::build(IR::VarRefExpr::build(*decl_size_ptr)),
         IR::ConstExpr::build(IR::UI<8>::build(1)),
         IR::ArithmeticExpr::Opcode::Add));

   // Add the statements to the program.
   builder.appendStmt(std::move(assign));
   builder.appendStmt(std::move(update_counter));
}

void FuseChunkSink::setUpState(const ExecutionContext& context) {
   state = std::make_unique<FuseChunkSinkState>();
   auto& col = context.getColumn(**source_ius.begin());
   state->raw_data = col.raw_data;
   state->size = &col.size;
}

void FuseChunkSink::tearDownState() {
   state.reset();
}

void* FuseChunkSink::accessState() const {
   return state.get();
}

std::string FuseChunkSink::id() const {
   return "FuseChunkSink" + (*source_ius.begin())->type->id();
}

}