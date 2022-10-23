#include "algebra/suboperators/sources/ScratchPadIUProvider.h"
#include "algebra/CompilationContext.h"

namespace inkfuse {

SuboperatorArc ScratchPadIUProvider::build(const RelAlgOp* source_, const IU& provided_iu) {
   return SuboperatorArc{new ScratchPadIUProvider(source_, provided_iu)};
}

ScratchPadIUProvider::ScratchPadIUProvider(const RelAlgOp* source_, const IU& provided_iu)
   : TemplatedSuboperator<EmptyState>(source_, {&provided_iu}, {}) {}

void ScratchPadIUProvider::open(CompilationContext& context) {
   auto& builder = context.getFctBuilder();
   const IU& out_iu = *provided_ius.front();
   // We attach all statements to the function preamble.
   std::deque<IR::StmtPtr> stmts;

   // Declare the target IU.
   auto& declare = stmts.emplace_back(IR::DeclareStmt::build(context.buildIUIdentifier(out_iu), out_iu.type));
   context.declareIU(out_iu, *declare);
   // Allocate backing memory for the target IU.
   auto fct = context.getRuntimeFunction("inkfuse_malloc");
   // Warning: this could not be done for a suboperator which supports interpretation,
   // as the actual number of bytes might be unknown at compilation time. 
   std::vector<IR::ExprPtr> args;
   args.push_back(IR::ConstExpr::build(IR::UI<8>::build(out_iu.type->numBytes())));
   auto alloc_call = IR::InvokeFctExpr::build(*fct, std::move(args));
   stmts.emplace_back(IR::AssignmentStmt::build(*declare, std::move(alloc_call)));

   // Add statements to the function preamble.
   builder.getRootBlock().appendStmts(std::move(stmts));

   // And notify consumer that the IU is ready.
   context.notifyIUsReady(*this);
}

}