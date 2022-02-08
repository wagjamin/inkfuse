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
   CompilationContext(std::string program_name, Pipeline& pipeline_);

   /// Compile the backing pipeline.
   void compile();

   /// Resolve the scope of a sub-operator.
   size_t resolveScope(const Suboperator& op);

   /// Notify the compilation context that the IUs of a given sub-operator are ready to be consumed.
   void notifyIUsReady(const Suboperator& op);
   /// Request a specific IU.
   void requestIU(const Suboperator& op, IUScoped iu);

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
   static IR::FunctionBuilder createFctBuilder(IR::IRBuilder& program);

   struct Builder {
      Builder(IR::Program& program);

      IR::IRBuilder ir_builder;
      IR::FunctionBuilder fct_builder;
   };

   /// The pipeline in whose context we generate the code.
   Pipeline& pipeline;
   /// The backing IR program.
   IR::Program program;
   /// The function builder for the generated code.
   std::optional<Builder> builder;
   /// Which sub-operators were computed already? Needed to prevent double-computation in DAGs.
   std::set<const Suboperator*> computed;
   /// How many of the IU requests were serviced for every operator.
   std::unordered_map<const Suboperator*, size_t> serviced_requests;
   /// The open IU requests which need to be mapped. (from -> to)
   std::unordered_map<const Suboperator*, std::pair<const Suboperator*, const IU*>> requests;
   /// Scoped IU declarations.
   std::map<std::pair<const IU*, size_t>, const IR::Stmt*> scoped_declarations;
};

}

#endif //INKFUSE_COMPILATIONCONTEXT_H
