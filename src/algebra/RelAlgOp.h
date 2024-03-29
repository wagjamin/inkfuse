#ifndef INKFUSE_RELALGOP_H
#define INKFUSE_RELALGOP_H

#include "algebra/IU.h"
#include <vector>
#include <unordered_set>

namespace inkfuse {

struct PipelineDAG;

/// Relational algebra operator producing a set of IUs.
/// As we prepare a relational algebra query for execution, it "decays" into a DAG suitable of suboperators.
///
/// Let's look at this from a compiler perspective - inkfuse is effectively a compiler lowering to progressively
/// lower level IRs. relational algebra level -> suboperators -> inkfuse IR -> C.
/// This would be pretty sweet to build into MLIR - but we decided against it because we thought the
/// initial overhead might be overkill.
struct RelAlgOp {
   RelAlgOp(std::vector<std::unique_ptr<RelAlgOp>> children_, std::string op_name_ = "")
      : children(std::move(children_)), op_name(std::move(op_name_)) {}

   virtual ~RelAlgOp() = default;

   /// Transform the relational algebra operator into a DAG of suboperators.
   virtual void decay(PipelineDAG& dag) const = 0;

   /// Get the operator output.
   const std::vector<const IU*>& getOutput() const;

   /// Get the children of this operator.
   const std::vector<std::unique_ptr<RelAlgOp>>& getChildren() const;
   const std::string& getName() const;

   protected:
   /// Child operators.
   std::vector<std::unique_ptr<RelAlgOp>> children;
   /// Output IUs.
   std::vector<const IU*> output_ius;
   /// The name of this operator.
   std::string op_name;
};

using RelAlgOpPtr = std::unique_ptr<RelAlgOp>;

}

#endif //INKFUSE_RELALGOP_H
