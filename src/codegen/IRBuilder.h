#ifndef INKFUSE_IRBUILDER_H
#define INKFUSE_IRBUILDER_H

#include "codegen/Type.h"
#include <deque>

namespace inkfuse {

namespace IR {

struct Program;
struct Function;
struct Block;
struct IRBuilder;
struct Expr;
struct FunctionBuilder;
struct Stmt;
using BlockPtr = std::unique_ptr<Block>;
using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;
using FunctionArc = std::shared_ptr<Function>;

// Helper struct for building an if statement.
struct If {
   public:
   If(If&& other);
   If& operator=(If&& other) noexcept;

   // Enter the else block.
   void Else();

   // End the if statement.
   void End();

   // Destructor. Ends statement if not done yet.
   ~If();

   private:
   friend class FunctionBuilder;

   If(FunctionBuilder& builder_, ExprPtr expr_);

   /// Backing function builder.
   FunctionBuilder* builder;
   /// Original block of the backing builder.
   Block* original_block;
   /// ExpressionOp to be evaluated in the if.
   ExprPtr expr;
   /// If Block.
   BlockPtr if_block;
   /// Else Block
   BlockPtr else_block;
   /// Is the statement finished?
   bool finished = false;
   bool else_entered = false;
};

// Helper struct for building a while statement.
struct While {
   public:
   While(While&& other);
   While& operator=(While&& other) noexcept;

   // End the while statement.
   void End();

   // Destructor. Ends statement if not done yet.
   ~While();

   private:
   friend class FunctionBuilder;

   While(FunctionBuilder& builder_, ExprPtr expr_);

   /// Backing function builder.
   FunctionBuilder* builder;
   /// Original block of the backing builder.
   Block* original_block;
   /// ExpressionOp to be evaluated in the while.
   ExprPtr expr;
   /// While body.
   BlockPtr body;
   /// Is the statement finished?
   bool finished = false;
};

struct FunctionBuilder {
   public:
   /// Destructor, finalizes if the function was not yet finalized.
   ~FunctionBuilder();

   /// Append a statement to the current block being built.
   Stmt& appendStmt(StmtPtr stmt);

   /// Build an if statement being evaluated on the given expression.
   If buildIf(ExprPtr expr);

   /// Build a while statement being evaluated on the given expression.
   While buildWhile(ExprPtr expr);

   /// Get the statement at a given index.
   const Stmt& getArg(size_t idx) const;

   /// Get the current block.
   Block& getCurrBlock();
   /// Get the root block of this function.
   Block& getRootBlock();

   /// Wrap up code generation and add the function to the owning Program.
   void finalize();

   private:
   friend class IRBuilder;
   friend class If;
   friend class While;

   FunctionBuilder(IRBuilder& builder_, FunctionArc function);

   /// The overall function being created (i.e. name, types, and the main block).
   FunctionArc function;
   /// The current block being built. Effectively allows for "scoping" in the Function Builder.
   Block* curr_block;
   /// The root block of the function.
   Block* root_block;
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
