#ifndef INKFUSE_IR_H
#define INKFUSE_IR_H

#include "codegen/IRBuilder.h"
#include "codegen/Statement.h"
#include <deque>
#include <string>
#include <utility>
#include <vector>

/// This file contains all the central building blocks for the InkFuse IR.
/// The purpose of the InkFuse IR is to make generating code easier and support different compiler backends.
/// It allows for the creation of a high-level (C-like) IR in SSA form.
///
/// This is just a toy IR compared to some of the real DB IRs out there. A good example is the NoisePage IR
/// developed at CMU - take a look!
///
/// This IR can be later lowered to C. If we find that C is not performant enough wrt. compilation time
/// it will allow us to switch to LLVM codegen without changing the actual code generation logic.
namespace inkfuse {

namespace IR {

struct Block;
struct Program;
using BlockPtr = std::unique_ptr<Block>;
/// Programs can be referenced in many places. The best example is the global pre-amble
/// provided by the runtime system containing runtime structs and external functions.
using ProgramArc = std::shared_ptr<Program>;

/// IR function getting a set of parameters and returning a result.
struct Function {
   /// Create a new function, throws if not all of the arguments are pointing to declare statements.
   Function(std::string name_, std::vector<StmtPtr> arguments_, std::vector<bool> const_args, TypeArc return_type_);

   /// The unique function name.
   std::string name;

   /// The arguments. All of the statements are Declare statements.
   std::vector<StmtPtr> arguments;

   /// Which of the arguments are const?
   std::vector<bool> const_args;

   /// Return type of the function.
   TypeArc return_type;

   /// Get the function body (if it exists). Note that code can only be generated through the IRBuilder.
   const Block* getBody() const;

   private:
   friend class FunctionBuilder;

   /// Optional function body. If this is not set, we assume the function is external
   /// and will be provided during linking. This is needed for runtime functions provided by
   /// e.g. hash tables.
   BlockPtr body;
};

using FunctionArc = std::shared_ptr<Function>;

/// A block of statements within a function.
struct Block {
   Block(std::deque<StmtPtr> statements_) : statements(std::move(statements_)){};

   /// Append statement to the block.
   void appendStmt(StmtPtr stmt);
   /// Appends statements to the block.
   void appendStmts(std::deque<StmtPtr> stmts);

   /// Prepend statement to the block.
   void prependStmts(StmtPtr stmt);
   /// Prepends statements to the block.
   void prependStmts(std::deque<StmtPtr> stmts);

   /// Statements within the block.
   std::deque<StmtPtr> statements;
};

/// Central IR program which is a set of functions and structs.
/// Cannot be used to generate code directly, this must go through the IRBuilder class.
struct Program {
   /// Constructor, you have to provide a name for the program.
   /// If standalone is set, the global runtime definitions are not included by default (testing).
   Program(std::string program_name_, bool standalone = false);

   /// Get an IR builder for this program.
   IRBuilder getIRBuilder();

   /// The program name - leads to a globally unique path on the file system.
   const std::string program_name;

   const std::vector<ProgramArc>& getIncludes() const;

   const std::vector<StructArc>& getStructs() const;

   /// Get a struct type (or throw if it does not exist).
   TypeArc getStruct(std::string_view name) const;

   /// Get a function (or throw if it does not exist).
   FunctionArc getFunction(std::string_view name) const;

   const std::vector<FunctionArc>& getFunctions() const;

   private:
   friend class IRBuilder;

   /// Other programs this program depends on. Usually only the global runtime.
   std::vector<ProgramArc> includes;

   /// A set of structs defined within the program.
   std::vector<StructArc> structs;

   /// A set of functions defined within the program.
   std::vector<FunctionArc> functions;
};

} // namespace IR

} // namespace inkfuse

#endif //INKFUSE_IR_H
