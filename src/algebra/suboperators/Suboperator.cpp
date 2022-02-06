#include "algebra/suboperators/Suboperator.h"
#include "algebra/RelAlgOp.h"

namespace inkfuse {

void Suboperator::produce(CompilationContext& context) const
{
   auto scope = context.resolveScope(*this);
   // Request the creation of all source ius this suboperator needs to consume.
   for (const auto& src_iu: source_ius) {
      context.requestIU(*this, {*src_iu, scope});
   }
}

std::stringstream Suboperator::getVarIdentifier() const
{
   std::stringstream str;
   if (source) {
      str << "iu_" << source->op_name << "_" << this;
   } else {
      str << "iu" << this;
   }
   return str;
}

}