#ifndef INKFUSE_INDEXEDIUPROVIDER_H
#define INKFUSE_INDEXEDIUPROVIDER_H

#include "algebra/suboperators/Suboperator.h"
#include "algebra/CompilationContext.h"
#include "codegen/IRBuilder.h"

namespace inkfuse {

/// Runtime state to get the IndexedIUProvider running.
struct IndexedIUProviderState {
   static const char* name;

   /// Pointer to the raw data of the underlying column to be read.
   void* raw_data;
};

/// An indexed IU provider is a suboperator which receives an index
/// as input IU and then resolves a new IU through indexed offsets into some
/// backing contiguous storage section.
/// Examples are providing IUs from table scans and fuse chunks. The actual
/// offset logic behaves the same, just the state setup and loop drivers are different.
template <class RuntimeParams>
struct IndexedIUProvider : public TemplatedSuboperator<IndexedIUProviderState, RuntimeParams> {

   void consume(const IU& iu, CompilationContext& context) override {
      assert(&iu == *this->source_ius.begin());
      auto& builder = context.getFctBuilder();
      const auto& program = context.getProgram();

      const auto& loop_idx = context.getIUDeclaration({**this->source_ius.begin(), 0});

      const IR::Stmt* decl_data_ptr;
      {
         // In a first step we get the raw data pointer and extract it into the root scope.
         auto state_expr = context.accessGlobalState(*this);
         // Cast it to a TScanDriverState pointer.
         auto cast_expr = IR::CastExpr::build(std::move(state_expr), IR::Pointer::build(program.getStruct(IndexedIUProviderState::name)));
         // Build data variable.
         auto data_var_name = this->getVarIdentifier();
         data_var_name << "_start";
         auto target_ptr_type = IR::Pointer::build((*this->provided_ius.begin())->type);
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
      IUScoped declared_iu{**this->provided_ius.begin(), 0};
      auto declare = IR::DeclareStmt::build(this->buildIUName(declared_iu), (*this->provided_ius.begin())->type);
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
   };

   std::string id() const override {
      return providerName() + (*this->provided_ius.begin())->type->id();
   };

   protected:
   IndexedIUProvider(const RelAlgOp* source, const IU& driver_iu, const IU& produced_iu)
      : TemplatedSuboperator<IndexedIUProviderState, RuntimeParams>(source, {&produced_iu}, {&driver_iu}) {}

   /// Specific name of a provider needed to generate unique fragment ids.
   virtual std::string providerName() const = 0;

};

/// Runtime state for all IndexedIU providers is the same, meaning that
/// we only have to register the stuff once.
namespace IndexedIUProviderRuntime {
/// Register runtime structs and functions.
void registerRuntime();
}

}

#endif //INKFUSE_INDEXEDIUPROVIDER_H
