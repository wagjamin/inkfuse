#include "algebra/suboperators/Suboperator.h"
#include "algebra/CompilationContext.h"
#include "algebra/RelAlgOp.h"

namespace inkfuse {

void Suboperator::produce(CompilationContext& context) const
{
   auto scope = context.resolveScope(*this);
   // TODO HACK
   scope = 0;
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

std::string Suboperator::buildIUName(IUScoped iu) const
{
   std::stringstream str;
   str << "iu_" << iu.iu.name << "_" << iu.scope_id;
   return str.str();
}



}