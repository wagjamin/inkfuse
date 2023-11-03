#include "algebra/ExpressionOp.h"
#include "algebra/Pipeline.h"
#include "algebra/suboperators/expressions/ExpressionSubop.h"
#include "algebra/suboperators/expressions/RuntimeExpressionSubop.h"

#include <unordered_set>

namespace inkfuse {

IR::TypeArc ExpressionOp::derive(ComputeNode::Type code, const std::vector<Node*>& nodes) {
   std::vector<IR::TypeArc> types;
   std::for_each(nodes.begin(), nodes.end(), [&](Node* node) {
      types.push_back(node->output_type);
   });
   return derive(code, types);
}

IR::TypeArc ExpressionOp::derive(ComputeNode::Type code, const std::vector<IR::TypeArc>& types) {
   // Operations that return a boolean as result.
   static std::unordered_set<ComputeNode::Type> bool_returning{
      ComputeNode::Type::Eq, ComputeNode::Type::Neq,
      ComputeNode::Type::Less, ComputeNode::Type::LessEqual,
      ComputeNode::Type::Greater, ComputeNode::Type::GreaterEqual,
      ComputeNode::Type::StrEquals, ComputeNode::Type::And,
      ComputeNode::Type::Or};
   if (code == ComputeNode::Type::Hash) {
      return IR::UnsignedInt::build(8);
   }
   if (bool_returning.contains(code)) {
      return IR::Bool::build();
   }
   // TODO(benjamin) Unify type derivation rules with the raw codegen::Expression.
   return types[0];
}

ExpressionOp::ExpressionOp(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string op_name_, std::vector<Node*> out_, std::vector<NodePtr> nodes_)
   : RelAlgOp(std::move(children_), std::move(op_name_)), out(std::move(out_)), nodes(std::move(nodes_)) {
   for (auto node : out) {
      auto casted = dynamic_cast<ComputeNode*>(node);
      assert(casted);
      output_ius.push_back(&casted->out);
   }
}

std::unique_ptr<ExpressionOp> ExpressionOp::build(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string op_name_, std::vector<Node*> out_, std::vector<NodePtr> nodes_) {
   return std::make_unique<ExpressionOp>(std::move(children_), std::move(op_name_), std::move(out_), std::move(nodes_));
}

ExpressionOp::IURefNode::IURefNode(const IU* child_)
   : Node(child_->type), child(child_) {
}

ExpressionOp::ComputeNode::ComputeNode(Type code_, std::vector<Node*> children_)
   : Node(derive(code_, children_)), code(code_), out(output_type), children(std::move(children_)) {
   assert(code != Type::Constant && code != Type::Cast);
}

ExpressionOp::ComputeNode::ComputeNode(IR::TypeArc casted, Node* child)
   : Node(casted), code(Type::Cast), out(std::move(casted)), children({child}) {
}

ExpressionOp::ComputeNode::ComputeNode(Type code_, IR::ValuePtr arg_1, Node* arg_2)
   : Node(derive(code_, {arg_2})), code(code_), out(output_type), children({arg_2}), opt_runtime_param(std::move(arg_1)) {
   assert(code != Type::Hash && code != Type::Cast);
}

void ExpressionOp::decay(PipelineDAG& dag) const {
   // First decay the children.
   for (const auto& child : children) {
      child->decay(dag);
   }

   // The set stores which expression suboperators were created already.
   std::unordered_map<Node*, const IU*> built;
   for (const auto root : out) {
      decayNode(root, built, dag);
   }
}

void ExpressionOp::decayNode(Node* node, std::unordered_map<Node*, const IU*>& built, PipelineDAG& dag) const {
   if (built.count(node)) {
      // Stop early if this node was processed already.
      return;
   }

   if (auto ref_node = dynamic_cast<IURefNode*>(node)) {
      built[node] = ref_node->child;
   } else if (auto compute_node = dynamic_cast<ComputeNode*>(node)) {
      std::vector<const IU*> source_ius;
      built[node] = &compute_node->out;
      for (const auto child : compute_node->children) {
         // Decay all children.
         decayNode(child, built, dag);
         source_ius.push_back(built[child]);
      }
      std::vector<const IU*> out_ius{&compute_node->out};
      SuboperatorArc subop;
      if (!compute_node->opt_runtime_param) {
         // Add a regular ExpressionSubop for this node.
         subop = std::make_shared<ExpressionSubop>(this, std::move(out_ius), std::move(source_ius), compute_node->code);
      } else {
         // Add a RuntimeExpressionSubop for this node.
         subop = std::make_shared<RuntimeExpressionSubop>(this, std::move(out_ius), std::move(source_ius), compute_node->code, (*compute_node->opt_runtime_param)->getType());
         // Add the runtime parameters needed for the runtime expression.
         RuntimeExpressionParams params;
         params.dataSet((*compute_node->opt_runtime_param)->copy());
         static_cast<RuntimeExpressionSubop&>(*subop).attachRuntimeParams(std::move(params));
      }
      dag.getCurrentPipeline().attachSuboperator(std::move(subop));
   }
}

} // namespace inkfuse
