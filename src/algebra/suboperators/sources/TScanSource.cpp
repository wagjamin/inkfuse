#include "algebra/suboperators/sources/TScanSource.h"
#include "algebra/RelAlgOp.h"
#include "codegen/Type.h"
#include "exec/FuseChunk.h"
#include "runtime/Runtime.h"

namespace inkfuse {

const char* TScanDriverState::name = "TScanDriverState";
const char* TScanIUProviderState::name = "TScanIUProviderState";

// static
void TScanDriver::registerRuntime() {
   RuntimeStructBuilder{TScanDriverState::name}
      .addMember("start", IR::UnsignedInt::build(8))
      .addMember("end", IR::UnsignedInt::build(8));
}

void TSCanIUProvider::registerRuntime() {
   RuntimeStructBuilder{TScanIUProviderState::name}
      .addMember("start", IR::Pointer::build(IR::Void::build()));
}

std::unique_ptr<TScanDriver> TScanDriver::build(const RelAlgOp* source) {
   return std::unique_ptr<TScanDriver>(new TScanDriver(source));
}

TScanDriver::TScanDriver(const RelAlgOp* source)
   : Suboperator(source, {}, {}), loop_driver_iu(IR::UnsignedInt::build(8)) {
   if (source && loop_driver_iu.name.empty()) {
      loop_driver_iu.name = "iu_" + source->op_name + "_idx";
   }
}


void TScanDriver::attachRuntimeParams(TScanDriverRuntimeParams runtime_params_)
{
   runtime_params = runtime_params_;
}

void TScanDriver::produce(CompilationContext& context) const {
   auto& builder = context.getFctBuilder();
   const auto& program = context.getProgram();

   IR::Stmt* decl_start_ptr;
   IR::Stmt* decl_end_ptr;
   {
      // In a first step we get the start and end value from the picked morsel and extract them into the root scope.
      auto state_expr = context.accessGlobalState(*this);
      // Cast it to a TScanDriverState pointer.
      auto cast_expr = IR::CastExpr(std::move(state_expr), IR::Pointer::build(program.getStruct(TScanDriverState::name)));
      // Build start and end variables. Start variable is also the index IU.
      auto end_var_name = getVarIdentifier();
      end_var_name << "_end";
      auto decl_start = IR::DeclareStmt::build(loop_driver_iu.name, IR::UnsignedInt::build(8));
      decl_start_ptr = decl_start.get();
      auto decl_end = IR::DeclareStmt::build(end_var_name.str(), IR::UnsignedInt::build(8));
      decl_end_ptr = decl_end.get();
      // And assign.
      auto assign_start = IR::AssignmentStmt::build(*decl_start, IR::StructAccesExpr::build(IR::VarRefExpr::build(*decl_start), "start"));
      auto assign_end = IR::AssignmentStmt::build(*decl_end, IR::StructAccesExpr::build(IR::VarRefExpr::build(*decl_start), "end"));
      // Add this to the function preamble.
      std::deque<IR::StmtPtr> preamble_stmts;
      preamble_stmts.push_back(std::move(decl_start));
      preamble_stmts.push_back(std::move(decl_end));
      preamble_stmts.push_back(std::move(assign_start));
      preamble_stmts.push_back(std::move(assign_end));
      builder.getRootBlock().appendStmts(std::move(preamble_stmts));
   }

   {
      // Next up we create the driving for-loop.
      auto while_block = builder.buildWhile(
         IR::ArithmeticExpr::build(
            IR::VarRefExpr::build(*decl_start_ptr),
            IR::VarRefExpr::build(*decl_end_ptr),
            IR::ArithmeticExpr::Opcode::Less));
      // Alright, the loop variable is ready.
      context.notifyIUsReady(*this);
      // And update the loop counter
      auto increment = IR::AssignmentStmt::build(
         *decl_start_ptr,
         IR::ArithmeticExpr::build(
            IR::VarRefExpr::build(*decl_start_ptr),
            IR::ConstExpr::build(IR::UI<8>::build(1)),
            IR::ArithmeticExpr::Opcode::Less
            )
         );
      builder.appendStmt(std::move(increment));
      while_block.End();
   }
}

bool TScanDriver::pickMorsel() {
   assert(state);

   if (!first_picked) {
      state->start = 0;
      first_picked = true;
   } else {
      state->start = state->end;
   }

   // Go up to the maximum chunk size of the intermediate results or the total relation size.
   state->end = std::min(state->start + DEFAULT_CHUNK_SIZE, runtime_params->rel_size);

   // If the starting point advanced to the end, then we know there are no more morsels to pick.
   return state->start != runtime_params->rel_size;
}

void TScanDriver::setUpState() {
   assert(runtime_params);
   state = std::make_unique<TScanDriverState>();
}

void TScanDriver::tearDownState() {
   state.reset();
}

void* TScanDriver::accessState() const {
   return state.get();
}

void TSCanIUProvider::attachRuntimeParams(TScanIUProviderRuntimeParams runtime_params_)
{
   runtime_params = runtime_params_;
}

void TSCanIUProvider::consume(IURef iu, CompilationContext& context) const
{
   assert(&iu.get() == *source_ius.begin());
   // Declare IU.

}

void TSCanIUProvider::setUpState()
{
   assert(runtime_params);
   state = std::make_unique<TScanIUProviderState>();
   state->raw_data = runtime_params->raw_data;
}

void TSCanIUProvider::tearDownState()
{
   state.reset();
}

void * TSCanIUProvider::accessState() const
{
   return state.get();
}

std::string TSCanIUProvider::id() const
{
   return "TSCanIUProvider_" + params.type->id();
}

TSCanIUProvider::TSCanIUProvider(const RelAlgOp* source, IURef driver_iu, IURef produced_iu)
: Suboperator(source, {&produced_iu.get()}, {&driver_iu.get()})
{

}

}