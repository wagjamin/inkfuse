#ifndef INKFUSE_IRBUILDER_H
#define INKFUSE_IRBUILDER_H

#include "codegen/Type.h"

namespace inkfuse {

namespace IR {

struct Program;
struct Function;
struct Block;
struct IRBuilder;
struct Expr;
struct Stmt;
using BlockPtr = std::unique_ptr<Block>;
using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;
using FunctionArc = std::shared_ptr<Function>;

struct FunctionBuilder {
   public:
   /// Destructor, finalizes if the function was not yet finalized.
   ~FunctionBuilder();

   /// Add a statement to the current block being built.
   void appendStmt(StmtPtr stmt);

   /// Get the statement at a given index.
   const Stmt& getArg(size_t idx);

   /// Wrap up code generation and add the function to the owning Program.
   void finalize();

   private:
   friend class IRBuilder;

   FunctionBuilder(IRBuilder& builder_, FunctionArc function);

   /// The overall function being created (i.e. name, types, and the main block).
   FunctionArc function;
   /// The current block being built. Effectively allows for "scoping" in the Function Builder.
   Block* curr_block;
   /// The backing IR builder.
   IRBuilder& builder;
   /// Was the function finalized already.
   bool finalized = false;
};

struct IRBuilder {
   public:
   /// Add a struct to the backing program.
   void addStruct(StructArc new_struct);

   /// Add given function to the program.
   void addFunction(FunctionArc function);

   /// Create a function builder for the given function header.
   /// The body of the function should be empty.
   FunctionBuilder createFunctionBuilder(FunctionArc function);

   private:
   friend class Program;

   IRBuilder(Program& program_);

   /// Backing program for which we generate code.
   Program& program;
};

}

};

#endif //INKFUSE_IRBUILDER_H
