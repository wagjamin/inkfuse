#ifndef INKFUSE_EXPRESSIONFRAGMENTIZER_H
#define INKFUSE_EXPRESSIONFRAGMENTIZER_H

#include "interpreter/FragmentGenerator.h"

namespace inkfuse {

struct ExpressionFragmentizer: public Fragmentizer {

   ExpressionFragmentizer();

   private:
   /// Fragmentize all cast expressions.
   void fragmentizeCasts();
   /// Fragmentize all binary expressions.
   void fragmentizeBinary();

};

}

#endif //INKFUSE_EXPRESSIONFRAGMENTIZER_H
