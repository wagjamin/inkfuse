#include "codegen/Statement.h"
#include "codegen/IR.h"

namespace inkfuse {

namespace IR {

IfStmt::IfStmt(ExprPtr expr_, BlockPtr if_block_, BlockPtr else_block_)
   : expr(std::move(expr_)), if_block(std::move(if_block_)), else_block(std::move(else_block_)) {
}

WhileStmt::WhileStmt(ExprPtr expr_, BlockPtr while_block_)
   : expr(std::move(expr_)), while_block(std::move(while_block_)) {
}

StmtPtr DeclareStmt::build(std::string name_, TypeArc type_) {
   return std::make_unique<DeclareStmt>(std::move(name_), std::move(type_));
}

StmtPtr AssignmentStmt::build(const Stmt& stmt, ExprPtr expr_) {
   return std::make_unique<AssignmentStmt>(stmt, std::move(expr_));
}

StmtPtr AssignmentStmt::build(ExprPtr left_side_, ExprPtr expr_)
{
   return std::make_unique<AssignmentStmt>(std::move(left_side_), std::move(expr_));
}

}

}