#ifndef INKFUSE_EXPRESSIONHELPERS_H
#define INKFUSE_EXPRESSIONHELPERS_H

#include "algebra/suboperators/expressions/ExpressionSubop.h"
#include "codegen/Expression.h"

#include <unordered_map>

namespace inkfuse {

namespace ExpressionHelpers {

using Type = ExpressionOp::ComputeNode::Type;

/// Actual names of the operations for id generation.
extern const std::unordered_map<Type, std::string> expr_names;

/// Map from algebra expression types to IR expressions in the jitted code.
extern const std::unordered_map<Type, IR::ArithmeticExpr::Opcode> code_map;

}

}

#endif //INKFUSE_EXPRESSIONHELPERS_H
