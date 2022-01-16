#ifndef INKFUSE_STATEMENT_H
#define INKFUSE_STATEMENT_H

#include "codegen/Expression.h"
#include "codegen/Type.h"
#include <exception>
#include <memory>
#include <string>

/// This file contains the basic statements within the InkFuse IR.
namespace inkfuse {

namespace IR {

/// Forward declarations.
struct Block;
using BlockPtr = std::unique_ptr<Block>;

/// Basic IR statement.
struct Stmt {
   /// Virtual base destructor.
   virtual ~Stmt() = default;
};

using StmtPtr = std::unique_ptr<Stmt>;

/// Function invocation statement.
struct InvokeFctStmt : public Stmt {
   InvokeFctStmt(ExprPtr invoke_fct_expr_) : invoke_fct_expr(std::move(invoke_fct_expr_)) {
      if (!dynamic_cast<IR::InvokeFctExpr*>(invoke_fct_expr.get())) {
         throw std::runtime_error("cannot wrap non invoke expression in invoke statement");
      }
   }

   /// Backing expression for invoking the function.
   ExprPtr invoke_fct_expr;
};

/// Variable declaration statement.
struct DeclareStmt : public Stmt {
   DeclareStmt(std::string name_, TypeArc type_) : name(std::move(name_)), type(std::move(type_)) {}
   static StmtPtr build(std::string name_, TypeArc type_);

   /// Variable name.
   std::string name;
   /// Variable type.
   TypeArc type;
};

/// Assignment statement.
struct AssignmentStmt : public Stmt {
   AssignmentStmt(const Stmt& statement, ExprPtr expr_) : declaraion(reinterpret_cast<const DeclareStmt&>(statement)), expr(std::move(expr_)) {}
   static StmtPtr build(const Stmt& statement, ExprPtr expr_);

   /// Variable name to which to write.
   const DeclareStmt& declaraion;
   /// Expression which should be assigned.
   ExprPtr expr;
};

/// Return statement.
struct ReturnStmt : public Stmt {
   static StmtPtr build(ExprPtr expr_) {
      return std::make_unique<ReturnStmt>(ReturnStmt{std::move(expr_)});
   }

   /// Expression to be evaluated, the result gets returned.
   ExprPtr expr;

   private:
   ReturnStmt(ExprPtr expr_) : expr(std::move(expr_)){};
};

/// If statement.
struct IfStmt : public Stmt {
   IfStmt(ExprPtr expr_, BlockPtr if_block_, BlockPtr else_block_);

   /// Expression to evaluate within the if statement.
   ExprPtr expr;
   /// If block.
   BlockPtr if_block;
   /// Optional else block.
   BlockPtr else_block;
};

/// While statement.
struct WhileStmt : public Stmt {
   WhileStmt(ExprPtr expr_, BlockPtr while_block_);

   /// Expression to evaluate to decide whether to enter the while loop.
   ExprPtr expr;
   /// Block within the while statement.
   BlockPtr while_block;
};

/// Statement visitor utility.
template <typename Arg>
struct StmtVisitor {
   public:
   bool visit(const Stmt& stmt, Arg arg) {
      if (const auto elem = dynamic_cast<const InvokeFctStmt*>(&stmt)) {
         return visitInvokeFct(*elem, arg);
      } else if (auto elem = dynamic_cast<const DeclareStmt*>(&stmt)) {
         return visitDeclare(*elem, arg);
      } else if (auto elem = dynamic_cast<const AssignmentStmt*>(&stmt)) {
         return visitAssignment(*elem, arg);
      } else if (auto elem = dynamic_cast<const IfStmt*>(&stmt)) {
         return visitIf(*elem, arg);
      } else if (auto elem = dynamic_cast<const WhileStmt*>(&stmt)) {
         return visitWhile(*elem, arg);
      } else if (auto elem = dynamic_cast<const ReturnStmt*>(&stmt)) {
         return visitReturn(*elem, arg);
      } else {
         return false;
      }
   }

   private:
   virtual bool visitInvokeFct(const InvokeFctStmt& type, Arg arg) { return false; }

   virtual bool visitDeclare(const DeclareStmt& type, Arg arg) { return false; }

   virtual bool visitAssignment(const AssignmentStmt& type, Arg arg) { return false; }

   virtual bool visitIf(const IfStmt& type, Arg arg) { return false;}

   virtual bool visitWhile(const WhileStmt& type, Arg arg) { return false; }

   virtual bool visitReturn(const ReturnStmt& type, Arg arg) { return false; }
};

}

}

#endif //INKFUSE_STATEMENT_H
