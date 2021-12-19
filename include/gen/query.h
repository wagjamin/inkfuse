
#pragma once

#include "gen/tpcc.h"
#include "imlab/database.h"
#include "imlab/infra/hash_table.h"
#include "imlab/storage/relation.h"
#include "imlab/infra/types.h"
#include <tuple>
#include <map>
#include <iostream>

namespace imlab {

struct GeneratedQuery {
    static void executeOLAPQuery(Database& db);
};

} // namespace imlab

