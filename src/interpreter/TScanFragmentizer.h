#ifndef INKFUSE_COLUMNSCANFRAGMENTIZER_H
#define INKFUSE_COLUMNSCANFRAGMENTIZER_H

#include "algebra/Pipeline.h"
#include "algebra/suboperators/sources/TableScanSource.h"
#include "interpreter/FragmentGenerator.h"
#include <list>

namespace inkfuse {

struct TScanFragmetizer : public Fragmentizer {

   TScanFragmetizer();

};

}

#endif //INKFUSE_COLUMNSCANFRAGMENTIZER_H
