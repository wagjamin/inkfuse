#include "algebra/TableScan.h"
#include "algebra/Pipeline.h"
#include "algebra/suboperators/sources/TableScanSource.h"
#include "exec/FuseChunk.h"

namespace inkfuse {

TableScan::TableScan(StoredRelation& rel_, std::vector<std::string> cols_, std::string name)
   : RelAlgOp({}, std::move(name)), rel(rel_)
{
   for(auto& col: cols_) {
      // Set up the IUs.
      std::string iu_name;
      if (!name.empty()) {
         iu_name = name + "_" + col;
      } else {
         iu_name = col;
      }
      IU iu(rel.getColumn(col).getType(), std::move(iu_name));
      auto& elem = cols.emplace_back(std::make_pair(std::move(col), std::move(iu)));
      output_ius.push_back(&elem.second);
   }
}

void TableScan::decay(PipelineDAG& dag) const
{
   // Create a new pipeline.
   auto& pipe = dag.buildNewPipeline();
   // Set up the loop driver.
   auto& driver = reinterpret_cast<TScanDriver&>(pipe.attachSuboperator(TScanDriver::build(this)));
   TScanDriverRuntimeParams params {rel.getColumn(0).second.length()};
   driver.attachRuntimeParams(params);
   auto driver_iu = *driver.getIUs().begin();
   // Set up the actual column scans.
   for (auto& col: cols) {
      // Attach the operator.
      auto& provider = reinterpret_cast<TScanIUProvider&>(pipe.attachSuboperator(TScanIUProvider::build(this, *driver_iu, col.second)));
      auto& data_col = rel.getColumn(col.first);
      TScanIUProviderRuntimeParams iu_params
         {
            .raw_data = data_col.getRawData(),
         };
      provider.attachRuntimeParams(iu_params);
   }
}

}