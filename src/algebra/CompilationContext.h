#ifndef INKFUSE_COMPILATIONCONTEXT_H
#define INKFUSE_COMPILATIONCONTEXT_H

#include "algebra/Pipeline.h"
#include "codegen/IR.h"
#include "codegen/Value.h"
#include "exec/FuseChunk.h"

#include <cstdint>
#include <optional>
#include <memory>
#include <unordered_map>
#include <map>
#include <set>

namespace inkfuse {

struct Suboperator;

/// Context for compiling a single pipeline.
struct CompilationContext {
   /// Set up a compilation context for generating code for a given pipeline.
   CompilationContext(std::string program_name, const Pipeline& pipeline_);
   /// Set up a compilation context which will generate the code within a specific IR program.
   CompilationContext(IR::ProgramArc program_, std::string fct_name_, const Pipeline& pipeline_);

   /// Compile the backing pipeline.
   void compile();

   /// Resolve the scope of a sub-operator.
   size_t resolveScope(const Suboperator& op);

   /// Notify the compilation context that the IUs of a given sub-operator are ready to be consumed.
   void notifyIUsReady(Suboperator& op);
   /// Request a specific IU.
   void requestIU(Suboperator& op, const IU& iu);
   /// Notify that one of the operators closed.
   void notifyOpClosed(Suboperator& op);

   /// Declare an IU within the context of the given suboperator.
   void declareIU(IUScoped iu, const IR::Stmt& stmt);
   /// Get an IU declaration.
   const IR::Stmt& getIUDeclaration(IUScoped iu);

   /// Access the local state of the given sub-operator. Returns a void pointer.
   IR::ExprPtr accessGlobalState(const Suboperator& op);

   /// Get the current function builder.
   const IR::Program& getProgram();
   IR::FunctionBuilder& getFctBuilder();

   private:
   static IR::FunctionBuilder createFctBuilder(IR::IRBuilder& program, std::string fct_name);

   struct Builder {
      Builder(IR::Program& program, std::string fct_name);

      IR::IRBuilder ir_builder;
      IR::FunctionBuilder fct_builder;
   };

   struct NodeProperties {
      /// How many of the children IUs were serviced already?
      size_t serviced_requests = 0;
      /// How many requests for this operator's IUs were there?
      size_t upstream_requests = 0;
   };

   /// The pipeline in whose context we generate the code.
   const Pipeline& pipeline;
   /// The function name.
   std::string fct_name;
   /// The backing IR program.
   IR::ProgramArc program;
   /// The function builder for the generated code.
   std::optional<Builder> builder;
   /// Which sub-operators were computed already? Needed to prevent double-computation in DAGs.
   std::set<const Suboperator*> computed;
   /// Properties of the different nodes in the suboperator dag.
   std::unordered_map<Suboperator*, NodeProperties> properties;
   /// The open IU requests which need to be mapped. (from -> to)
   std::unordered_map<Suboperator*, std::pair<Suboperator*, const IU*>> requests;
   /// Scoped IU declarations.
   std::map<std::pair<const IU*, size_t>, const IR::Stmt*> scoped_declarations;
};

}

#endif //INKFUSE_COMPILATIONCONTEXT_H
