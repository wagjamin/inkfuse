#include "interpreter/RuntimeExpressionFragmentizer.h"
#include "algebra/ExpressionOp.h"
#include "algebra/suboperators/expressions/RuntimeExpressionSubop.h"

namespace inkfuse {

namespace {
using Type = ExpressionOp::ComputeNode::Type;

const std::vector<IR::TypeArc> types =
   TypeDecorator()
      .attachNumeric()
      .produce();

const std::vector<Type> op_types {
   Type::Add,
   Type::Subtract,
   Type::Multiply,
   Type::Divide,
   Type::Eq,
   Type::Less,
   Type::LessEqual,
   Type::Greater,
   Type::GreaterEqual,
};
}

RuntimeExpressionFragmentizer::RuntimeExpressionFragmentizer()
{
   // All binary operations on the same type.
   for (auto& type : types) {
      for (auto operation: op_types) {
         auto& [name, pipe] = pipes.emplace_back();
         auto& iu_1 = generated_ius.emplace_back(type, "");
         auto& iu_out = generated_ius.emplace_back(ExpressionOp::derive(operation, {type, type}), "");
         auto& op = pipe.attachSuboperator(RuntimeExpressionSubop::build(nullptr, {&iu_out}, {&iu_1}, operation, type));
         name = op.id();
      }
   }
}

}
