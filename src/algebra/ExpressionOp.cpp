#include "algebra/ExpressionOp.h"
#include "algebra/Pipeline.h"
#include "algebra/suboperators/expressions/ExpressionSubop.h"

#include <unordered_set>

namespace inkfuse {

IR::TypeArc ExpressionOp::derive(ComputeNode::Type code, const std::vector<Node*>& nodes)
{
   std::vector<IR::TypeArc> types;
   std::for_each(nodes.begin(), nodes.end(), [&](Node* node) {
      types.push_back(node->output_type);
   });
   return derive(code, types);
}

IR::TypeArc ExpressionOp::derive(ComputeNode::Type code, const std::vector<IR::TypeArc>& types)
{
   if (code == ComputeNode::Type::Hash) {
      return IR::UnsignedInt::build(8);
   }
   if (code == ComputeNode::Type::Eq
       || code == ComputeNode::Type::Less
       || code == ComputeNode::Type::LessEqual
       || code == ComputeNode::Type::Greater
       || code == ComputeNode::Type::GreaterEqual) {
      return IR::Bool::build();
   }
   // TODO Unify type derivation rules with the raw codegen::Expression.
   return types[0];
}

ExpressionOp::ExpressionOp(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string op_name_, std::vector<Node*> out_, std::vector<NodePtr> nodes_)
   : RelAlgOp(std::move(children_), std::move(op_name_)), out(std::move(out_)), nodes(std::move(nodes_)) {
   for (auto node: out) {
      auto casted = dynamic_cast<ComputeNode*>(node);
      assert(casted);
      output_ius.push_back(&casted->out);
   }
}

ExpressionOp::ConstantNode::ConstantNode(IR::ValuePtr val)
   : Node(val->getType()), value(std::move(val)), iu(val->getType()) {
}

ExpressionOp::IURefNode::IURefNode(const IU* child_)
   : Node(child_->type), child(child_) {
}

ExpressionOp::ComputeNode::ComputeNode(Type code_, std::vector<Node*> children_)
   : Node(derive(code_, children_)), code(code_), out(output_type), children(std::move(children_)) {
}

ExpressionOp::ComputeNode::ComputeNode(IR::TypeArc casted, Node* child)
   : Node(casted), code(Type::Cast), out(std::move(casted)), children({child})
{
}

void ExpressionOp::decay(PipelineDAG& dag) const {
   // First decay the children.
   for (const auto& child: children) {
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

} // namespace inkfuse
