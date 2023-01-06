#ifndef INKFUSE_LOOPDRIVER_H
#define INKFUSE_LOOPDRIVER_H

#include "algebra/CompilationContext.h"
#include "algebra/RelAlgOp.h"
#include "algebra/suboperators/Suboperator.h"
#include "codegen/IRBuilder.h"
#include <optional>

namespace inkfuse {

/// Runtime state of a given loop driver.
struct LoopDriverState {
   static const char* name;

   /// Index of the first tuple to read (inclusive).
   uint64_t start;
   /// Index of the last tuple to read (exclusive).
   uint64_t end;
};

struct LoopDriver : public TemplatedSuboperator<LoopDriverState> {
   /// A raw LoopDriver should never be interpreted. It only makes sense connected to the
   /// followup IU provider.
   bool outgoingStrongLinks() const override { return true; }

   /// Source - only open and close are relevant and create the respective loop driving execution.
   void open(CompilationContext& context) override {
      auto& builder = context.getFctBuilder();
      const auto& program = context.getProgram();

      IR::Stmt* decl_start_ptr;
      IR::Stmt* decl_end_ptr;
      {
         // TODO(benjamin) not very clean - do in similar way as HashTableSource::open
         // In a first step we get the start and end value from the picked morsel and extract them into the root scope.
         auto state_expr_1 = context.accessGlobalState(*this);
         auto state_expr_2 = context.accessGlobalState(*this);
         // Cast it to a TScanDriverState pointer.
         auto start_cast_expr = IR::CastExpr::build(std::move(state_expr_1), IR::Pointer::build(program.getStruct(LoopDriverState::name)));
         auto end_cast_expr = IR::CastExpr::build(std::move(state_expr_2), IR::Pointer::build(program.getStruct(LoopDriverState::name)));
         // Build start and end variables. Start variable is also the index IU.
         auto end_var_name = this->getVarIdentifier();
         end_var_name << "_end";
         auto driver_iu_name = context.buildIUIdentifier(loop_driver_iu);
         auto decl_start = IR::DeclareStmt::build(driver_iu_name, IR::UnsignedInt::build(8));
         decl_start_ptr = decl_start.get();
         auto decl_end = IR::DeclareStmt::build(end_var_name.str(), IR::UnsignedInt::build(8));
         decl_end_ptr = decl_end.get();
         // And assign.
         auto assign_start = IR::AssignmentStmt::build(*decl_start, IR::StructAccessExpr::build(std::move(start_cast_expr), "start"));
         auto assign_end = IR::AssignmentStmt::build(*decl_end, IR::StructAccessExpr::build(std::move(end_cast_expr), "end"));
         // Add this to the function preamble.
         std::deque<IR::StmtPtr> preamble_stmts;
         preamble_stmts.push_back(std::move(decl_start));
         preamble_stmts.push_back(std::move(decl_end));
         preamble_stmts.push_back(std::move(assign_start));
         preamble_stmts.push_back(std::move(assign_end));
         builder.getRootBlock().appendStmts(std::move(preamble_stmts));
      }

      // Register provided IU with the compilation context.
      context.declareIU(loop_driver_iu, *decl_start_ptr);

      // Next up we create the driving for-loop.
      this->opt_while = builder.buildWhile(
         IR::ArithmeticExpr::build(
            IR::VarRefExpr::build(*decl_start_ptr),
            IR::VarRefExpr::build(*decl_end_ptr),
            IR::ArithmeticExpr::Opcode::Less));
      {
         // Generate code for downstream consumers.
         context.notifyIUsReady(*this);
      }
   };

   void close(CompilationContext& context) override {
      // And update the loop counter
      auto& decl_start = context.getIUDeclaration(loop_driver_iu);
      auto increment = IR::AssignmentStmt::build(
         decl_start,
         IR::ArithmeticExpr::build(
            IR::VarRefExpr::build(decl_start),
            IR::ConstExpr::build(IR::UI<8>::build(1)),
            IR::ArithmeticExpr::Opcode::Add));
      context.getFctBuilder().appendStmt(std::move(increment));
      opt_while->End();
      opt_while.reset();
   };

   protected:
   LoopDriver(const RelAlgOp* source_)
      : TemplatedSuboperator<LoopDriverState>(source_, {}, {}), loop_driver_iu(IR::UnsignedInt::build(8)) {
      if (this->source && loop_driver_iu.name.empty()) {
         loop_driver_iu.name = this->source->getName() + "_idx";
      } else {
         loop_driver_iu.name = "driver_idx";
      }
      this->provided_ius.push_back(&loop_driver_iu);
   }

   /// Loop counter IU provided by the TScanDriver.
   IU loop_driver_iu;
   /// In-flight while loop being generated between calls to open() and close()
   std::optional<IR::While> opt_while;
};

/// Runtime state for all LoopDriver providers is the same, meaning that
/// we only have to register the stuff once.
namespace LoopDriverRuntime {
/// Register runtime structs and functions.
void registerRuntime();
}

}

#endif //INKFUSE_LOOPDRIVER_H
