#ifndef INKFUSE_TABLESCAN_H
#define INKFUSE_TABLESCAN_H

#include "algebra/RelAlgOp.h"
#include "storage/Relation.h"
#include <list>

namespace inkfuse {

/// A table scan relational operator. Reads a set of columns from an underlying relation
/// and makes them available as IUs.
struct TableScan : public RelAlgOp {
   TableScan(StoredRelation& rel_, std::vector<std::string> cols, std::string name);

   void decay(std::set<const IU*> required, PipelineDAG& dag) const override;

   protected:
   void addIUs(std::set<const IU*>& set) const override;

   private:
   // The relation which to read from.
   StoredRelation& rel;
   // Columns to be read.
   std::list<std::pair<std::string, IU>> cols;
};

}

#endif //INKFUSE_TABLESCAN_H
