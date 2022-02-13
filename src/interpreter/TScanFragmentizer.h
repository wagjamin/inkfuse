#ifndef INKFUSE_COLUMNSCANFRAGMENTIZER_H
#define INKFUSE_COLUMNSCANFRAGMENTIZER_H

#include "algebra/Pipeline.h"
#include "algebra/suboperators/sources/TableScanSource.h"
#include "interpreter/FragmentGenerator.h"
#include <list>

namespace inkfuse {

struct TScanFragmetizer : public Fragmentizer {

   TScanFragmetizer();

   const std::list<std::pair<std::string, Pipeline>>& getFragments() const override;

   private:
   /// The pipelines, one for every tscan type.
   std::list<std::pair<std::string, Pipeline>> pipes;
   /// IUs which were generated for the pipelines.
   std::list<IU> generated_ius;
};

}

#endif //INKFUSE_COLUMNSCANFRAGMENTIZER_H
