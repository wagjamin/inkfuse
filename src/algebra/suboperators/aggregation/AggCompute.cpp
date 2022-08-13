#include "algebra/suboperators/aggregation/AggCompute.h"

namespace inkfuse {

AggCompute::AggCompute(IR::TypeArc type_)
   : type(std::move(type_)) {
}

}
