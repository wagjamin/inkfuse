#include "algebra/ExpressionOp.h"
#include "algebra/Pipeline.h"
#include "algebra/suboperators/ExpressionSubop.h"

#include <unordered_set>

namespace inkfuse {

ExpressionOp::ExpressionOp(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string op_name_, std::vector<Node*> out_, std::vector<NodePtr> nodes_)
   : RelAlgOp(std::move(children_), std::move(op_name_)), out(std::move(out_)), nodes(std::move(nodes_)) {
}

ExpressionOp::ConstantNode::ConstantNode(IR::ValuePtr val)
   : value(std::move(val)), iu(val->getType()) {
}

ExpressionOp::IURefNode::IURefNode(const IU* child_)
   : child(child_) {
}

ExpressionOp::ComputeNode::ComputeNode(Type code_, std::vector<Node*> children_)
   : code(code_), out(IR::Void::build()), children(std::move(children_)) {
   // Derive result type.
   if (auto c_node = dynamic_cast<ConstantNode*>(children[0])) {
      out = IU(c_node->value->getType());
   } else if (auto r_node = dynamic_cast<IURefNode*>(children[0])) {
      out = IU(r_node->child->type);
   } else {
      auto compute_node = dynamic_cast<ComputeNode*>(children[0]);
      out = IU(compute_node->out);
   }
}

void ExpressionOp::decay(std::vector<const IU*> required, PipelineDAG& dag) const {
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

   if (auto c_node = dynamic_cast<ConstantNode*>(node)) {
      // TODO support
      throw std::runtime_error("constant expressions not implemented");
   } else if (auto r_node = dynamic_cast<IURefNode*>(node)) {
      built[node] = r_node->child;
   } else {
      std::vector<const IU*> source_ius;
      auto compute_node = dynamic_cast<ComputeNode*>(node);
      built[node] = &compute_node->out;
      assert(compute_node);
      for (const auto child : compute_node->children) {
         // Decay all children.
         decayNode(child, built, dag);
         source_ius.push_back(built[child]);
      }
      // And add the suboperator for this node.
      std::vector<const IU*> out_ius{&compute_node->out};
      auto subop = std::make_shared<ExpressionSubop>(this, std::move(out_ius), std::move(source_ius), compute_node->code);
      dag.getCurrentPipeline().attachSuboperator(std::move(subop));
   }
}

void ExpressionOp::addIUs(std::vector<const IU*>& vec) const {
   // Only add those IUs which actually get produced.
   for (const auto& node : out) {
      auto compute_node = dynamic_cast<ComputeNode*>(node);
      vec.push_back(&compute_node->out);
   }
}

} // namespace inkfuse
