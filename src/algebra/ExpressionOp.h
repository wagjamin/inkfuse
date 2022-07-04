#ifndef INKFUSE_EXPRESSIONOP_H
#define INKFUSE_EXPRESSIONOP_H

#include "algebra/RelAlgOp.h"
#include "codegen/Value.h"
#include <unordered_set>
#include <vector>

namespace inkfuse {

/// Relational algebra operator for evaluating a set of expressions.
struct ExpressionOp : public RelAlgOp {
   struct Node {
      virtual ~Node() = default;
      Node(IR::TypeArc output_type_): output_type(std::move(output_type_)) {};

      const IR::TypeArc output_type;
   };

   using NodePtr = std::unique_ptr<Node>;

   /// Reference to a provided IU.
   struct IURefNode : public Node {
      IURefNode(const IU* child_);

      const IU* child;
   };

   /// Single compute node which is part of a larger expression tree.
   struct ComputeNode : public Node {
      /// ExpressionOp types.
      enum class Type {
         Add,
         Cast,
         Hash,
         Subtract,
         Multiply,
         Divide,
         Eq,
         Less,
         LessEqual,
         Greater,
         GreaterEqual,
      };

      // Constructor for regular binary operations.
      ComputeNode(Type code, std::vector<Node*> children);
      // Constructor for cast operations.
      ComputeNode(IR::TypeArc casted, Node* child);
      // Constructor for lazy operation.
      ComputeNode(Type code, Node* arg_1, IR::ValuePtr arg_2);

      // Which expression?
      Type code;
      // Output IU on this node.
      IU out;
      // Children of this expression. Pointers are useful for DAG-shaped expression trees.
      std::vector<Node*> children;
      // Optional constant second argument if this is a lazy node.
      std::optional<IR::ValuePtr> opt_lazy;
   };

   ExpressionOp(
      std::vector<std::unique_ptr<RelAlgOp>> children_,
      std::string op_name_,
      std::vector<Node*>
         out_,
      std::vector<NodePtr>
         nodes_);

   void decay(PipelineDAG& dag) const override;

   /// Derive the output type of an expression.
   static IR::TypeArc derive(ComputeNode::Type code, const std::vector<Node*>& nodes);
   /// Derive the output type of an expression.
   static IR::TypeArc derive(ComputeNode::Type code, const std::vector<IR::TypeArc>& types);

   protected:

   /// Helper for decaying the expression DAG without duplicate subops.
   void decayNode(
      Node* node,
      std::unordered_map<Node*, const IU*>& built,
      PipelineDAG& dag) const;

   private:
   // Output nodes which actually generate columns.
   std::vector<Node*> out;
   /// Compute nodes which have to be evaluated.
   std::vector<NodePtr> nodes;
};

}

#endif //INKFUSE_EXPRESSIONOP_H
