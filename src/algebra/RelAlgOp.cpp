#include "algebra/RelAlgOp.h"

namespace inkfuse {

std::set<const IU*> RelAlgOp::getIUs() const
{
   std::set<const IU*> set;
   addIUs(set);
   return set;
}

std::set<const IU*> RelAlgOp::getIUsRecursive() const
{
   std::set<const IU*> set;
   addIUsRecursive(set);
   return set;
}

void RelAlgOp::addIUsRecursive(std::set<const IU*>& set) const
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