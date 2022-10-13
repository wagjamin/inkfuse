#include "algebra/suboperators/expressions/ExpressionHelpers.h"

namespace inkfuse {

namespace ExpressionHelpers {
/// Actual names of the operations for id generation.
const std::unordered_map<Type, std::string> expr_names{
   {Type::Add, "add"},
   {Type::Cast, "cast"},
   {Type::Subtract, "sub"},
   {Type::Multiply, "mul"},
   {Type::Hash, "hash"},
   {Type::Divide, "div"},
   {Type::Greater, "gt"},
   {Type::GreaterEqual, "ge"},
   {Type::Less, "lt"},
   {Type::LessEqual, "le"},
   {Type::Eq, "eq"},
   {Type::Neq, "neq"},
   {Type::StrEquals, "streq"}
};

/// Map from algebra expression types to IR expressions in the jitted code.
const std::unordered_map<Type, IR::ArithmeticExpr::Opcode> code_map{
   {Type::Add, IR::ArithmeticExpr::Opcode::Add},
   {Type::Subtract, IR::ArithmeticExpr::Opcode::Subtract},
   {Type::Multiply, IR::ArithmeticExpr::Opcode::Multiply},
   {Type::Divide, IR::ArithmeticExpr::Opcode::Divide},
   {Type::Greater, IR::ArithmeticExpr::Opcode::Greater},
   {Type::GreaterEqual, IR::ArithmeticExpr::Opcode::GreaterEqual},
   {Type::Less, IR::ArithmeticExpr::Opcode::Less},
   {Type::LessEqual, IR::ArithmeticExpr::Opcode::LessEqual},
   {Type::Eq, IR::ArithmeticExpr::Opcode::Eq},
   {Type::Neq, IR::ArithmeticExpr::Opcode::Neq},
   {Type::StrEquals, IR::ArithmeticExpr::Opcode::StrEquals}
};

}

}