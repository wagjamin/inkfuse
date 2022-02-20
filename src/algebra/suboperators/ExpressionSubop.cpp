#include "algebra/suboperators/ExpressionSubop.h"
#include <sstream>

namespace inkfuse {

namespace {

using Type = ExpressionOp::ComputeNode::Type;

const std::unordered_map<ExpressionOp::ComputeNode::Type, std::string> expr_names {
   {Type::Add, "add"},
   {Type::Subtract, "subtract"},
   {Type::Multiply, "mult"},
   {Type::Greater, "gt"},
   {Type::GreaterEqual, "ge"},
   {Type::Less, "lt"},
   {Type::LessEqual, "le"},
   {Type::Eq, "eq"}
};

}

ExpressionSubop::ExpressionSubop(const RelAlgOp* source_, std::vector<const IU*> provided_ius_, std::vector<const IU*> source_ius_, ExpressionOp::ComputeNode::Type type_)
   : TemplatedSuboperator<EmptyState, EmptyState>(source_, std::move(provided_ius_), std::move(source_ius_)), type(type_)
{
}

void ExpressionSubop::consumeAllChildren(CompilationContext& context)
{

}

std::string ExpressionSubop::id() const {
   std::stringstream res;
   res << "expr_" << expr_names.at(type);
   for (auto child: source_ius) {
      res << "_" << child->type->id();
   }
   return res.str();
}

}
