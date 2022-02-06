#ifndef INKFUSE_COMPILATIONCONTEXT_H
#define INKFUSE_COMPILATIONCONTEXT_H

#include "codegen/IR.h"
#include "codegen/Value.h"
#include "exec/FuseChunk.h"

#include <cstdint>
#include <optional>
#include <memory>
#include <unordered_map>
#include <set>

namespace inkfuse {

struct Suboperator;
struct Pipeline;

/// Context for compiling a single pipeline.
struct CompilationContext {
   /// Set up a compilation context for generating code for a given pipeline.
   CompilationContext(std::string program_name, Pipeline& pipeline_);

   /// Notify the compilation context that the IUs of a given sub-operator are ready to be consumed.
   void notifyIUsReady(const Suboperator& op);

   /// Request a specific IU from a given operator.
   void requestIU(const Suboperator& op, IURef req);

   /// Access the local state of the given sub-operator. Returns a void pointer.
   IR::ExprPtr accessGlobalState(const Suboperator& op);

   /// Get the current function builder.
   const IR::Program& getProgram();
   IR::FunctionBuilder& getFctBuilder();

   private:
   static IR::FunctionBuilder createFctBuilder(IR::Program& program);

   /// The pipeline in whose context we generate the code.
   Pipeline& pipeline;
   /// The backing IR program.
   IR::Program program;
   /// The function builder for the generated code.
   IR::FunctionBuilder fct_builder;
   /// Which suboperators were computed already? Needed to prevent double-computation in DAGs.
   std::set<Suboperator*> computed;
   /// The open IU requests which need to be mapped. (from -> to)
   std::unordered_map<const Suboperator*, std::pair<const Suboperator*, IURef>> requests;
   /// How many of the IU requests were serviced for every operator.
   std::unordered_map<const Suboperator*, size_t> serviced_requests;
};

}

#endif //INKFUSE_COMPILATIONCONTEXT_H
