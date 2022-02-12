#include "algebra/suboperators/sources/TScanSource.h"
#include "algebra/CompilationContext.h"
#include "algebra/RelAlgOp.h"
#include "codegen/Type.h"
#include "exec/FuseChunk.h"
#include "runtime/Runtime.h"
#include <functional>

namespace inkfuse {

const char* TScanDriverState::name = "TScanDriverState";
const char* TScanIUProviderState::name = "TScanIUProviderState";

// static
void TScanDriver::registerRuntime() {
   RuntimeStructBuilder{TScanDriverState::name}
      .addMember("start", IR::UnsignedInt::build(8))
      .addMember("end", IR::UnsignedInt::build(8));
}

void TScanIUProvider::registerRuntime() {
   RuntimeStructBuilder{TScanIUProviderState::name}
      .addMember("start", IR::Pointer::build(IR::Void::build()));
}

std::unique_ptr<TScanDriver> TScanDriver::build(const RelAlgOp* source) {
   return std::unique_ptr<TScanDriver>(new TScanDriver(source));
}

TScanDriver::TScanDriver(const RelAlgOp* source)
   : Suboperator(source, {}, {}), loop_driver_iu(IR::UnsignedInt::build(8)) {
   if (source && loop_driver_iu.name.empty()) {
      loop_driver_iu.name = "iu_" + source->getName() + "_idx";
   }
   provided_ius.emplace(&loop_driver_iu);
}


void TScanDriver::attachRuntimeParams(TScanDriverRuntimeParams runtime_params_)
{
   runtime_params = runtime_params_;
}

void TScanDriver::rescopePipeline(Pipeline& pipe)
{
   // Root scope will always have id 0.
   ScopePtr scope = std::make_unique<Scope>(0);
   pipe.rescope(std::move(scope));
}

void TScanDriver::open(CompilationContext& context) {
   auto& builder = context.getFctBuilder();
   const auto& program = context.getProgram();

   IR::Stmt* decl_start_ptr;
   IR::Stmt* decl_end_ptr;
   {
      // In a first step we get the start and end value from the picked morsel and extract them into the root scope.
      auto state_expr_1 = context.accessGlobalState(*this);
      auto state_expr_2 = context.accessGlobalState(*this);
      // Cast it to a TScanDriverState pointer.
      auto start_cast_expr = IR::CastExpr::build(std::move(state_expr_1), IR::Pointer::build(program.getStruct(TScanDriverState::name)));
      auto end_cast_expr = IR::CastExpr::build(std::move(state_expr_2), IR::Pointer::build(program.getStruct(TScanDriverState::name)));
      // Build start and end variables. Start variable is also the index IU.
      auto end_var_name = getVarIdentifier();
      end_var_name << "_end";
      auto decl_start = IR::DeclareStmt::build(loop_driver_iu.name, IR::UnsignedInt::build(8));
      decl_start_ptr = decl_start.get();
      auto decl_end = IR::DeclareStmt::build(end_var_name.str(), IR::UnsignedInt::build(8));
      decl_end_ptr = decl_end.get();
      // And assign.
      auto assign_start = IR::AssignmentStmt::build(*decl_start, IR::StructAccesExpr::build(std::move(start_cast_expr), "start"));
      auto assign_end = IR::AssignmentStmt::build(*decl_end, IR::StructAccesExpr::build(std::move(end_cast_expr), "end"));
      // Add this to the function preamble.
      std::deque<IR::StmtPtr> preamble_stmts;
      preamble_stmts.push_back(std::move(decl_start));
      preamble_stmts.push_back(std::move(decl_end));
      preamble_stmts.push_back(std::move(assign_start));
      preamble_stmts.push_back(std::move(assign_end));
      builder.getRootBlock().appendStmts(std::move(preamble_stmts));
   }

   // Register provided IU with the compilation context.
   context.declareIU({loop_driver_iu, 0}, *decl_start_ptr);

   {
      // Next up we create the driving for-loop.
      opt_while.emplace(builder.buildWhile(
         IR::ArithmeticExpr::build(
            IR::VarRefExpr::build(*decl_start_ptr),
            IR::VarRefExpr::build(*decl_end_ptr),
            IR::ArithmeticExpr::Opcode::Less)));
      // Generate code for downstream consumers.
      context.notifyIUsReady(*this);
   }
}

void TScanDriver::close(CompilationContext& context)
{
   // And update the loop counter
   auto& decl_start = context.getIUDeclaration(IUScoped{loop_driver_iu, 0});
   auto increment = IR::AssignmentStmt::build(
      decl_start,
      IR::ArithmeticExpr::build(
         IR::VarRefExpr::build(decl_start),
         IR::ConstExpr::build(IR::UI<8>::build(1)),
         IR::ArithmeticExpr::Opcode::Add
      )
   );
   context.getFctBuilder().appendStmt(std::move(increment));
   opt_while->End();
   opt_while.reset();
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
   first_picked = false;
}

void* TScanDriver::accessState() const {
   return state.get();
}

std::unique_ptr<TScanIUProvider> TScanIUProvider::build(const RelAlgOp* source, const IU& driver_iu, const IU& produced_iu)
{
   return std::unique_ptr<TScanIUProvider>(new TScanIUProvider{source, driver_iu, produced_iu});
}

void TScanIUProvider::attachRuntimeParams(TScanIUProviderRuntimeParams runtime_params_)
{
   runtime_params = runtime_params_;
}

void TScanIUProvider::consume(const IU& iu, CompilationContext& context)
{
   assert(&iu == *source_ius.begin());
   auto& builder = context.getFctBuilder();
   const auto& program = context.getProgram();

   const auto& loop_idx = context.getIUDeclaration({**source_ius.begin(), 0});

   const IR::Stmt* decl_data_ptr;
   {
      // In a first step we get the raw data pointer and extract it into the root scope.
      auto state_expr = context.accessGlobalState(*this);
      // Cast it to a TScanDriverState pointer.
      auto cast_expr = IR::CastExpr::build(std::move(state_expr), IR::Pointer::build(program.getStruct(TScanIUProviderState::name)));
      // Build data variable.
      auto data_var_name = getVarIdentifier();
      data_var_name << "_start";
      auto target_ptr_type = IR::Pointer::build(iu.type);
      auto decl_start = IR::DeclareStmt::build(data_var_name.str(), target_ptr_type);
      decl_data_ptr = decl_start.get();
      // And assign the casted raw pointer.
      auto assign_start = IR::AssignmentStmt::build(
         *decl_start,
         IR::CastExpr::build(
            IR::StructAccesExpr::build(std::move(cast_expr), "start"),
            target_ptr_type
            )
         );
      // Add this to the function preamble.
      std::deque<IR::StmtPtr> preamble_stmts;
      preamble_stmts.push_back(std::move(decl_start));
      preamble_stmts.push_back(std::move(assign_start));
      builder.getRootBlock().appendStmts(std::move(preamble_stmts));
   }

   // Declare IU.
   IUScoped declared_iu{**provided_ius.begin(), 0};
   auto declare = IR::DeclareStmt::build(buildIUName(declared_iu), (*provided_ius.begin())->type);
   context.declareIU(declared_iu, *declare);
   // Assign value to IU. This is done by adding the offset to the data pointer and dereferencing.
   auto assign = IR::AssignmentStmt::build(
      *declare,
      IR::DerefExpr::build(
         IR::ArithmeticExpr::build(
            IR::VarRefExpr::build(*decl_data_ptr),
            IR::VarRefExpr::build(loop_idx),
            IR::ArithmeticExpr::Opcode::Add
            )
         )
      );
   // Add the statements to the program.
   builder.appendStmt(std::move(declare));
   builder.appendStmt(std::move(assign));

   // And notify consumer that the IU is ready.
   context.notifyIUsReady(*this);
}

void TScanIUProvider::setUpState()
{
   assert(runtime_params);
   state = std::make_unique<TScanIUProviderState>();
   state->raw_data = runtime_params->raw_data;
}

void TScanIUProvider::tearDownState()
{
   state.reset();
}

void * TScanIUProvider::accessState() const
{
   return state.get();
}

std::string TScanIUProvider::id() const
{
   return "TSCanIUProvider_" + (*provided_ius.begin())->type->id();
}

TScanIUProvider::TScanIUProvider(const RelAlgOp* source, const IU& driver_iu, const IU& produced_iu)
: Suboperator(source, {&produced_iu}, {&driver_iu})
{
}

}