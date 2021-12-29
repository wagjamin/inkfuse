#ifndef INKFUSE_EXPRESSION_H
#define INKFUSE_EXPRESSION_H

#include "algebra/Suboperator.h"

namespace inkfuse {

    /// Basic expression sub-operator, taking a set of input IUs and producing one output IU.
    struct ExpressionSubop: public Suboperator {

        ExpressionSubop

    };

    struct BinaryArithmeticOp : public ExpressionSubop {

    };

    struct UnaryArithmeticOp : public ExpressionSubop {

    };

}

#endif //INKFUSE_EXPRESSION_H
