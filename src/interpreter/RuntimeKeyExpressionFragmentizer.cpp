#include "interpreter/RuntimeKeyExpressionFragmentizer.h"
#include "algebra/ExpressionOp.h"
#include "algebra/suboperators/expressions/RuntimeKeyExpressionSubop.h"

namespace inkfuse {

namespace {

using Type = ExpressionOp::ComputeNode::Type;

const std::vector<IR::TypeArc> types =
   TypeDecorator()
      .attachTypes()
      .produce();

}

RuntimeKeyExpressionFragmentizer::RuntimeKeyExpressionFragmentizer()
{
   for (auto& type: types) {
      auto& [name, pipe] = pipes.emplace_back();
      const auto& bool_out = generated_ius.emplace_back(IR::Bool::build(), "out_iu");
      const auto& data_in = generated_ius.emplace_back(type, "compare");
      const auto& ptr_in = generated_ius.emplace_back(IR::Pointer::build(IR::Char::build()), "data_ptrs");
      const auto& op = pipe.attachSuboperator(RuntimeKeyExpressionSubop::build(nullptr, bool_out, data_in, ptr_in));
      name = op.id();
   }
}

}