#ifndef INKFUSE_COMPILATIONCONTEXT_H
#define INKFUSE_COMPILATIONCONTEXT_H

#include "algebra/Pipeline.h"
#include "codegen/IR.h"
#include "codegen/Value.h"
#include "exec/FuseChunk.h"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>

namespace inkfuse {

struct Suboperator;

/// Hints that can be used during the code generation to generate more optimized
/// code. Examples:
///
/// When we are generating `OperatorFusing` code, we do not issue prefetch instructions.
/// These are exclusively used in the vectorized backends to issue independent loads
/// and hide cache miss latency for followup operators.
struct OptimizationHints {
   enum class CodegenMode {
      OperatorFusing,
      Vectorized,
   };

   CodegenMode mode = CodegenMode::OperatorFusing;
};

/// Context for compiling a single pipeline.
struct CompilationContext {
   /// Set up a compilation context for generating code for a full given pipeline.
   CompilationContext(std::string program_name, const Pipeline& pipeline_, OptimizationHints hints_ = OptimizationHints{});
   /// Set up a compilation context which will generate the code within a specific IR program for the full pipeline.
   CompilationContext(IR::ProgramArc program_, std::string fct_name_, const Pipeline& pipeline_, OptimizationHints hints_ = OptimizationHints{});

   /// Compile the code for this context.
   void compile();

   /// Notify the compilation context that the IUs of a given sub-operator are ready to be consumed.
   void notifyIUsReady(Suboperator& op);
   /// Request a specific IU.
   void requestIU(Suboperator& op, const IU& iu);
   /// Notify that one of the operators closed.
   void notifyOpClosed(Suboperator& op);

   /// Declare an IU within the context of the given suboperator.
   void declareIU(const IU& iu, const IR::Stmt& stmt);
   /// Get a unique IU identifier for a producing operator.
   std::string buildIUIdentifier(const IU& iu);
   /// Get an IU declaration.
   const IR::Stmt& getIUDeclaration(const IU& iu);

   /// Access the local state of the given sub-operator. Returns a void pointer.
   IR::ExprPtr accessGlobalState(const Suboperator& op) const;
   /// Get a function from the inkfuse runtime.
   IR::FunctionArc getRuntimeFunction(std::string_view name) const;

   /// Get the current function builder.
   const IR::Program& getProgram();
   IR::FunctionBuilder& getFctBuilder();

   /// Get the optimization hints for the generated program.
   const OptimizationHints& getOptimizationHints() const;

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
   const std::string fct_name;
   /// The backing IR program.
   IR::ProgramArc program;
   /// Optimization hints that can be used during code generation.
   OptimizationHints optimization_hints;
   /// The function builder for the generated code.
   std::optional<Builder> builder;
   /// Which sub-operators were computed already? Needed to prevent double-computation in DAGs.
   std::set<const Suboperator*> computed;
   /// Properties of the different nodes in the suboperator dag.
   std::unordered_map<Suboperator*, NodeProperties> properties;
   /// The open IU requests which need to be mapped. (from -> to)
   std::unordered_map<Suboperator*, std::pair<Suboperator*, const IU*>> requests;
   /// IU declarations.
   std::map<const IU*, const IR::Stmt*> iu_declarations;
};

}

#endif //INKFUSE_COMPILATIONCONTEXT_H
