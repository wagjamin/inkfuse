#include "algebra/suboperators/Suboperator.h"
#include "algebra/CompilationContext.h"
#include "algebra/RelAlgOp.h"

namespace inkfuse {

void Suboperator::open(CompilationContext& context)
{
   // Request the creation of all source ius this suboperator needs to consume.
   for (const auto& src_iu: source_ius) {
      context.requestIU(*this, *src_iu);
   }
}

void Suboperator::close(CompilationContext& context)
{
   context.notifyOpClosed(*this);
}

std::stringstream Suboperator::getVarIdentifier() const
{
   std::stringstream str;
   if (source) {
      str << "iu_" << source->getName() << "_" << this;
   } else {
      str << "iu" << this;
   }
   return str;
}

std::string Suboperator::buildIUName(IUScoped iu) const
{
   std::stringstream str;
   std::string name = iu.iu.name;
   std::replace(name.begin(), name.end(), '.', '_');
   str << "iu_" << name << "_" << iu.scope_id;
   return str.str();
}

}