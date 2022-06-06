#include "algebra/RelAlgOp.h"

namespace inkfuse {

const std::vector<const IU *> & RelAlgOp::getOutput() const
{
   return output_ius;
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