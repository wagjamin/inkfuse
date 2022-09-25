#ifndef INKFUSE_INDEXEDIUPROVIDER_H
#define INKFUSE_INDEXEDIUPROVIDER_H

#include "algebra/CompilationContext.h"
#include "algebra/suboperators/RuntimeParam.h"
#include "algebra/suboperators/Suboperator.h"
#include "algebra/suboperators/properties/WithRuntimeParams.h"
#include "codegen/IRBuilder.h"

namespace inkfuse {

/// Runtime state to get the IndexedIUProvider running.
struct IndexedIUProviderState {
   static const char* name;

   /// Pointer to the raw data of the underlying column to be read.
   void* raw_data;
   /// Runtime parameter if we are providing a parametrized type.
   uint64_t type_param;
};

/// Runtime parameter which gets resolved during runtime or code generation, depending on whether it is attached.
struct IndexedIUProviderRuntimeParam {
   RUNTIME_PARAM(type_param, IndexedIUProviderState);
};

/// An indexed IU provider is a suboperator which receives an index
/// as input IU and then resolves a new IU through indexed offsets into some
/// backing contiguous storage section.
/// Examples are providing IUs from table scans and fuse chunks. The actual
/// offset logic behaves the same, just the state setup and loop drivers are different.
struct IndexedIUProvider : public TemplatedSuboperator<IndexedIUProviderState>, public WithRuntimeParams<IndexedIUProviderRuntimeParam> {
   /// An IndexedIUProvider has to be understood in the context of its preceding LoopDriver.
   bool incomingStrongLinks() const override { return true; }

   void consume(const IU& iu, CompilationContext& context) override {
      assert(&iu == *this->source_ius.begin());
      auto& builder = context.getFctBuilder();
      const IU& source_iu = *source_ius.front();
      const IU& out_iu = *provided_ius.front();
      const auto& program = context.getProgram();
      const bool is_variable_size_type = (dynamic_cast<IR::ByteArray*>(out_iu.type.get()) != nullptr);

      const auto& loop_idx = context.getIUDeclaration(source_iu);

      const IR::Stmt* decl_data_ptr;
      // A custom iterator for dynamically sized types.
      const IR::Stmt* decl_custom_iterator;
      {
         // In a first step we get the raw data pointer and extract it into the root scope.
         auto state_expr = context.accessGlobalState(*this);
         // Cast it to a TScanDriverState pointer.
         auto cast_expr = IR::CastExpr::build(std::move(state_expr), IR::Pointer::build(program.getStruct(IndexedIUProviderState::name)));
         // Build data variable.
         auto data_var_name = this->getVarIdentifier();
         data_var_name << "_start";
         auto target_ptr_type = is_variable_size_type ? IR::Pointer::build(IR::Char::build()) : IR::Pointer::build(out_iu.type);
         auto decl_start = IR::DeclareStmt::build(data_var_name.str(), target_ptr_type);
         decl_data_ptr = decl_start.get();
         // And assign the casted raw pointer.
         auto assign_start = IR::AssignmentStmt::build(
            *decl_start,
            IR::CastExpr::build(
               IR::StructAccessExpr::build(std::move(cast_expr), "start"),
               target_ptr_type));
         // Add this to the function preamble.
         std::deque<IR::StmtPtr> preamble_stmts;
         preamble_stmts.push_back(std::move(decl_start));
         preamble_stmts.push_back(std::move(assign_start));
         if (is_variable_size_type) {
            // This is a dynamically sized type and we have to carry our own iterator.
            // Usually the loop index is enough to perform offsetting logic, but this is not the case
            // for dynamically sized types.
            auto iterator_name = this->getVarIdentifier();
            iterator_name << "_iterator";
            decl_custom_iterator = preamble_stmts.emplace_back(IR::DeclareStmt::build(iterator_name.str(), IR::UnsignedInt::build(8))).get();
            // Zero the iterator in the beginning.
            preamble_stmts.push_back(IR::AssignmentStmt::build(*decl_custom_iterator, IR::ConstExpr::build(IR::UI<8>::build(0))));
         }
         builder.getRootBlock().appendStmts(std::move(preamble_stmts));
      }

      // Declare IU.
      const auto& declared_iu = **this->provided_ius.begin();
      const auto& declare_stmt = builder.appendStmt(IR::DeclareStmt::build(context.buildIUIdentifier(declared_iu), (*this->provided_ius.begin())->type));
      context.declareIU(declared_iu, declare_stmt);

      // Assign value to IU. This is done by adding the index offset to the data pointer and dereferencing.
      // For statically sized types we can just use the loop index as iterator.
      if (is_variable_size_type) {
         auto assign = IR::ArithmeticExpr::build(
            IR::VarRefExpr::build(*decl_data_ptr),
            IR::VarRefExpr::build(*decl_custom_iterator),
            IR::ArithmeticExpr::Opcode::Add);
         builder.appendStmt(IR::AssignmentStmt::build(declare_stmt, std::move(assign)));
         // Update the custom iterator by incrementing by the type width.
         auto it_update = IR::ArithmeticExpr::build(
            IR::VarRefExpr::build(*decl_custom_iterator),
            runtime_params.type_paramResolve(*this, context),
            IR::ArithmeticExpr::Opcode::Add);
         builder.appendStmt(IR::AssignmentStmt::build(*decl_custom_iterator, std::move(it_update)));
      } else {
         auto assign =
            IR::DerefExpr::build(
               IR::ArithmeticExpr::build(
                  IR::VarRefExpr::build(*decl_data_ptr),
                  IR::VarRefExpr::build(loop_idx),
                  IR::ArithmeticExpr::Opcode::Add));
         builder.appendStmt(IR::AssignmentStmt::build(declare_stmt, std::move(assign)));
      }

      // And notify consumer that the IU is ready.
      context.notifyIUsReady(*this);
   };

   std::string id() const override {
      return providerName() + (*this->provided_ius.begin())->type->id();
   };

   protected:
   IndexedIUProvider(const RelAlgOp* source, const IU& driver_iu, const IU& produced_iu)
      : TemplatedSuboperator<IndexedIUProviderState>(source, {&produced_iu}, {&driver_iu}) {}

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
