#include "algebra/ExpressionOp.h"
#include "algebra/Pipeline.h"
#include "algebra/suboperators/expressions/ExpressionSubop.h"
#include "algebra/suboperators/expressions/LazyExpressionSubop.h"

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
   if (code == ComputeNode::Type::Hash) {
      return IR::UnsignedInt::build(8);
   }
   if (code == ComputeNode::Type::Eq || code == ComputeNode::Type::Less || code == ComputeNode::Type::LessEqual || code == ComputeNode::Type::Greater || code == ComputeNode::Type::GreaterEqual) {
      return IR::Bool::build();
   }
   // TODO Unify type derivation rules with the raw codegen::Expression.
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

ExpressionOp::IURefNode::IURefNode(const IU* child_)
   : Node(child_->type), child(child_) {
}

ExpressionOp::ComputeNode::ComputeNode(Type code_, std::vector<Node*> children_)
   : Node(derive(code_, children_)), code(code_), out(output_type), children(std::move(children_)) {
}

ExpressionOp::ComputeNode::ComputeNode(IR::TypeArc casted, Node* child)
   : Node(casted), code(Type::Cast), out(std::move(casted)), children({child}) {
}

ExpressionOp::ComputeNode::ComputeNode(Type code_, Node* arg_1, IR::ValuePtr arg_2)
   : Node(derive(code_, {arg_1})), code(code_), out(output_type), children({arg_1}), opt_lazy(std::move(arg_2)) {
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
      if (!compute_node->opt_lazy) {
         // Add a regular ExpressionSubop for this node.
         subop = std::make_shared<ExpressionSubop>(this, std::move(out_ius), std::move(source_ius), compute_node->code);
      } else {
         // Add a LazyExpressionSubop for this node.
         subop = std::make_shared<LazyExpressionSubop>(this, std::move(out_ius), std::move(source_ius), compute_node->code, (*compute_node->opt_lazy)->getType());
         // Add the runtime parameters needed for the lazy expression.
         LazyExpressionParams params;
         params.dataSet((*compute_node->opt_lazy)->copy());
         static_cast<LazyExpressionSubop&>(*subop).attachLazyParams(std::move(params));
      }
      dag.getCurrentPipeline().attachSuboperator(std::move(subop));
   }
}

} // namespace inkfuse
