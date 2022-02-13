#include "algebra/suboperators/sources/TScanSource.h"
#include "algebra/CompilationContext.h"
#include "algebra/RelAlgOp.h"
#include "codegen/Type.h"
#include "exec/FuseChunk.h"
#include "runtime/Runtime.h"
#include <functional>

namespace inkfuse {

const char* TScanDriverState::name = "TScanDriverState";

// static
void TScanDriver::registerRuntime() {
   RuntimeStructBuilder{TScanDriverState::name}
      .addMember("start", IR::UnsignedInt::build(8))
      .addMember("end", IR::UnsignedInt::build(8));
}

std::unique_ptr<TScanDriver> TScanDriver::build(const RelAlgOp* source) {
   return std::unique_ptr<TScanDriver>(new TScanDriver(source));
}

TScanDriver::TScanDriver(const RelAlgOp* source)
   : TemplatedSuboperator<TScanDriverState, TScanDriverRuntimeParams>(source, {}, {}), loop_driver_iu(IR::UnsignedInt::build(8)) {
   if (source && loop_driver_iu.name.empty()) {
      loop_driver_iu.name = "iu_" + source->getName() + "_idx";
   }
   provided_ius.emplace(&loop_driver_iu);
}

void TScanDriver::rescopePipeline(Pipeline& pipe)
{
   // Root scope will always have id 0. No selection vector.
   ScopePtr scope = std::make_unique<Scope>(0, nullptr);
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
   state->end = std::min(state->start + DEFAULT_CHUNK_SIZE, params->rel_size);

   // If the starting point advanced to the end, then we know there are no more morsels to pick.
   return state->start != params->rel_size;
}

void TScanIUProvider::setUpStateImpl(const ExecutionContext& context)
{
   state->raw_data = params->raw_data;
}

std::string TScanIUProvider::providerName() const
{
   return "TScanIUProvider";
}

std::unique_ptr<TScanIUProvider> TScanIUProvider::build(const RelAlgOp* source, const IU& driver_iu, const IU& produced_iu)
{
   return std::unique_ptr<TScanIUProvider>(new TScanIUProvider{source, driver_iu, produced_iu});
}


TScanIUProvider::TScanIUProvider(const RelAlgOp* source, const IU& driver_iu, const IU& produced_iu)
: IndexedIUProvider<TScanIUProviderRuntimeParams>(source, driver_iu, produced_iu)
{
}

}