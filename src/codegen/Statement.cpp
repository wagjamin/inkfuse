#include "codegen/Statement.h"
#include "codegen/IR.h"

namespace inkfuse {

namespace IR {

IfStmt::IfStmt(ExprPtr expr_, BlockPtr if_block_, BlockPtr else_block_)
   : expr(std::move(expr_)), if_block(std::move(if_block_)), else_block(std::move(else_block_)) {
}

WhileStmt::WhileStmt(ExprPtr expr_, BlockPtr if_block_)
   : expr(std::move(expr_)), if_block(std::move(if_block_)) {
}

}

}