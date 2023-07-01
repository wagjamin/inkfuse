#include "algebra/Print.h"
#include "algebra/Pipeline.h"
#include "algebra/suboperators/sinks/FuseChunkSink.h"
#include "exec/ExecutionContext.h"

namespace inkfuse {

PrettyPrinter::PrettyPrinter(std::vector<const IU*> ius_, std::vector<std::string> colnames_, std::optional<size_t> limit)
: ius(std::move(ius_)), colnames(std::move(colnames_)), limit(limit)
{
   if (ius.empty()) {
      throw std::runtime_error("PrettyPrinter must produce at least one column");
   }
   if (ius.size() != colnames.size()) {
      throw std::runtime_error("PrettyPrinter name/iu schema must have the same number of entries");
   }
}

bool PrettyPrinter::markMorselDone(inkfuse::ExecutionContext& ctx, size_t thread_id) {
   std::unique_lock lock(write_mut);
   // How many rows are we allowed to write until we hit the limit?
   auto chunk_size = ctx.getColumn(*ius[0], thread_id).size;
   size_t write = chunk_size;
   if (limit) {
      write = std::min(write, *limit);
      *limit = *limit - write;
   }
   num_rows += write;

   if (!out) {
      // Nothing we can write into.
      return limit && *limit == 0;
   }

   if (morsel_count == 0) {
      // This is the first morsel to finish - write the output header.
      *out << colnames[0];
      for (auto colname = colnames.begin() + 1; colname < colnames.end(); ++colname) {
         *out << ", " << *colname;
      }
      *out << "\n";
   }

   // Get the raw pointers to the data.
   std::vector<char*> data;
   data.reserve(ius.size());
   for (const IU* iu: ius) {
      auto& col = ctx.getColumn(*iu, thread_id);
      assert(col.size == chunk_size);
      data.push_back(col.raw_data);
   }
   // Write out the actual chunk.
   for (size_t row = 0; row < write; ++row) {
      for (size_t col = 0; col < ius.size(); ++col) {
         if (col != 0) {
            *out << ",";
         }
         const IU& iu = *ius[col];
         char* ptr = data[col];
         iu.type->print(*out, ptr + row * iu.type->numBytes());
      }
      *out << "\n";
   }
   morsel_count++;
   // If the limit reached 0 we close the stream.
   return limit && *limit == 0;
}

void PrettyPrinter::setOstream(std::ostream& ostream)
{
   out = &ostream;
}

std::unique_ptr<Print> Print::build(std::vector<std::unique_ptr<RelAlgOp>> children_, std::vector<const IU*> ius, std::vector<std::string> colnames, std::string op_name_, std::optional<size_t> limit)
{
   return std::unique_ptr<Print>{new Print(std::move(children_), std::move(ius), std::move(colnames), std::move(op_name_), limit)};
}

Print::Print(std::vector<std::unique_ptr<RelAlgOp>> children_, std::vector<const IU*> ius, std::vector<std::string> colnames, std::string op_name_, std::optional<size_t> limit)
   : RelAlgOp(std::move(children_), std::move(op_name_)), printer(std::make_unique<PrettyPrinter>(std::move(ius), std::move(colnames), limit))
{
}

void Print::decay(inkfuse::PipelineDAG& dag) const
{
   for (auto& child: children) {
      child->decay(dag);
   }

   auto& pipe = dag.getCurrentPipeline();
   // Build a FuseChunkSink to materialize each output IU.
   for (auto& out: printer->ius) {
      pipe.attachSuboperator(FuseChunkSink::build(this, *out));
   }
   // And attach the pretty-printer to the pipeline DAG.
   pipe.setPrettyPrinter(*printer);
}

}
