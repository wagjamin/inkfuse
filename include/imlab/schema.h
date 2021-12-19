// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------

#ifndef INCLUDE_IMLAB_SCHEMA_H_
#define INCLUDE_IMLAB_SCHEMA_H_

#include "imlab/infra/types.h"

namespace imlab {
namespace tpcc {

enum Relation {
   kWarehouse,
   kDistrict,
   kCustomer,
   kHistory,
   kNewOrder,
   kOrder,
   kOrderLine,
   kItem,
   kStock,
};

} // namespace tpcc
} // namespace imlab

#endif // INCLUDE_IMLAB_SCHEMA_H_
