#include "algebra/RelAlgOp.h"

namespace inkfuse {

std::vector<const IU*> RelAlgOp::getIUs() const
{
   std::vector<const IU*> vec;
   addIUs(vec);
   return vec;
}

std::vector<const IU*> RelAlgOp::getIUsRecursive() const
{
   std::vector<const IU*> vec;
   addIUsRecursive(vec);
   return vec;
}

void RelAlgOp::addIUsRecursive(std::vector<const IU*>& set) const
{
   addIUs(set);
   for (const auto& child: children) {
      child->addIUsRecursive(set);
   }
}

const std::vector<std::unique_ptr<RelAlgOp>>& RelAlgOp::getChildren() const
{
   return children;
}

const std::string& RelAlgOp::getName() const
{
   return op_name;
}

}