#include "interpreter/ExpressionFragmentizer.h"
#include "algebra/ExpressionOp.h"
#include "algebra/suboperators/expressions/ExpressionSubop.h"

namespace inkfuse {

namespace {
using Type = ExpressionOp::ComputeNode::Type;

const std::vector<IR::TypeArc> types =
   TypeDecorator()
      .attachTypes()
      .produce();

const std::vector<Type> op_types {
   Type::Add,
   Type::Subtract,
   Type::Multiply,
   Type::Divide,
   Type::Eq,
   Type::Neq,
   Type::Less,
   Type::LessEqual,
   Type::Greater,
   Type::GreaterEqual,
   Type::And,
   Type::Or
};
}

void ExpressionFragmentizer::fragmentizeCasts()
{
   // Cast all numeric types to all other numeric types.
   for (auto& type_in : types) {
      for (auto& type_out: types) {
         auto& [name, pipe] = pipes.emplace_back();
         auto& iu_in = generated_ius.emplace_back(type_in, "");
         auto& iu_out = generated_ius.emplace_back(type_out, "");
         auto& op = pipe.attachSuboperator(ExpressionSubop::build(nullptr, {&iu_out}, {&iu_in}, Type::Cast));
         name = op.id();
      }
   }
}

void ExpressionFragmentizer::fragmentizeBinary()
{
   // All binary operations on the same type.
   for (auto& type : types) {
      for (auto operation: op_types) {
         auto& [name, pipe] = pipes.emplace_back();
         auto& iu_1 = generated_ius.emplace_back(type, "");
         auto& iu_2 = generated_ius.emplace_back(type, "");
         auto& iu_out = generated_ius.emplace_back(ExpressionOp::derive(operation, {type, type}), "");
         auto& op = pipe.attachSuboperator(ExpressionSubop::build(nullptr, {&iu_out}, {&iu_1, &iu_2}, operation));
         name = op.id();
      }
   }
}

void ExpressionFragmentizer::fragmentizeFunctions()
{
   {
      // strcmp
      auto type = IR::String::build();
      auto& [name, pipe] = pipes.emplace_back();
      auto& iu_1 = generated_ius.emplace_back(type, "");
      auto& iu_2 = generated_ius.emplace_back(type, "");
      auto& iu_out = generated_ius.emplace_back(ExpressionOp::derive(Type::StrEquals, {type, type}), "");
      auto& op = pipe.attachSuboperator(ExpressionSubop::build(nullptr, {&iu_out}, {&iu_1, &iu_2}, Type::StrEquals));
      name = op.id();
   }  
}

ExpressionFragmentizer::ExpressionFragmentizer()
{
   fragmentizeBinary();
   fragmentizeCasts();
   fragmentizeFunctions();
}


}
