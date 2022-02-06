#include "codegen/IRBuilder.h"
#include "codegen/IR.h"
#include <memory>

namespace inkfuse {

namespace IR {

If::~If() {
   End();
}

If::If(FunctionBuilder& builder_, ExprPtr expr_)
   : builder(builder_),
     original_block(builder.curr_block),
     expr(std::move(expr_)),
     if_block(std::make_unique<Block>(std::vector<StmtPtr>{})),
     else_block(std::make_unique<Block>(std::vector<StmtPtr>{}))
{
   // Install if block as the current one in the builder.
   // Form now on, all appendStmts in the backing builder will go into this block.
   builder.curr_block = if_block.get();
}

void If::Else()
{
   if (finished || else_entered) {
      throw std::runtime_error("Cannot enter else twice");
   }

   // Install else block as the current one in the builder.
   builder.curr_block = else_block.get();

   // Else can only be entered once, otherwise we made a logical error.
   else_entered = true;
}

void If::End()
{
   if (finished) {
      return;
   }

   // Install original block again.
   builder.curr_block = original_block;
   // Create the if statement.
   StmtPtr stmt = std::make_unique<IfStmt>(std::move(expr), std::move(if_block), std::move(else_block));
   // Add it as the first one to the original builder.
   builder.appendStmt(std::move(stmt));
   // Finished can only be entered once.
   finished = true;
}

While::~While()
{
   End();
}

While::While(FunctionBuilder& builder_, ExprPtr expr_)
   : builder(builder_),
     original_block(builder.curr_block),
     expr(std::move(expr_)),
     body(std::make_unique<Block>(std::vector<StmtPtr>{}))
{
   // Install body block as the current one in the builder.
   // Form now on, all appendStmts in the backing builder will go into this block.
   builder.curr_block = body.get();
}

void While::End()
{
   if (finished) {
      return;
   }

   // Install original block again.
   builder.curr_block = original_block;
   // Create the while statement.
   StmtPtr stmt = std::make_unique<WhileStmt>(std::move(expr), std::move(body));
   // Add it as the first one to the original builder.
   builder.appendStmt(std::move(stmt));
   // Finished can only be entered once.
   finished = true;
}

FunctionBuilder::FunctionBuilder(IRBuilder& builder_, FunctionArc function_)
   : function(std::move(function_)), builder(builder_) {
   if (function->body) {
      throw std::runtime_error("Cannot create function builder on function with non-empty body");
   }
   function->body = std::make_unique<Block>(std::vector<StmtPtr>{});
   root_block = function->body.get();
   curr_block = root_block;
}

Stmt& FunctionBuilder::appendStmt(StmtPtr stmt) {
   if (!stmt) {
      throw std::runtime_error("cannot add emptry StmtPtr to function.");
   }
   auto& ret = *stmt;
   // Add a statement to the current block - note that this can be re-scoped by control flow blocks
   // like if statements and while loops.
   curr_block->statements.push_back(std::move(stmt));
   return ret;
}

If FunctionBuilder::buildIf(ExprPtr expr)
{
   return If{*this, std::move(expr)};
}

While FunctionBuilder::buildWhile(ExprPtr expr)
{
   return While{*this, std::move(expr)};
}

const Stmt& FunctionBuilder::getArg(size_t idx) {
   return *function->arguments.at(idx);
}

Block& FunctionBuilder::getCurrBlock()
{
   return *curr_block;
}

Block& FunctionBuilder::getRootBlock()
{
   return *root_block;
}

FunctionBuilder::~FunctionBuilder() {
   finalize();
}

void FunctionBuilder::finalize() {
   if (!finalized) {
      // Add the function to the backing IR program.
      finalized = true;
      builder.addFunction(std::move(function));
   }
}

IRBuilder::IRBuilder(Program& program_) : program(program_) {
}

void IRBuilder::addStruct(StructArc new_struct) {
   program.structs.push_back(std::move(new_struct));
}

void IRBuilder::addFunction(FunctionArc function) {
   program.functions.push_back(std::move(function));
}

FunctionBuilder IRBuilder::createFunctionBuilder(FunctionArc function) {
   return FunctionBuilder{*this, std::move(function)};
}

}

}
