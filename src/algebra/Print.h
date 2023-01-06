#ifndef INKFUSE_PRINT_H
#define INKFUSE_PRINT_H

#include "algebra/RelAlgOp.h"
#include <optional>
#include <ostream>

namespace inkfuse {

struct ExecutionContext;

/// The pretty printer that gets created by a Print operator.
struct PrettyPrinter {
   PrettyPrinter(std::vector<const IU*> ius, std::vector<std::string> colnames, std::optional<size_t> limit);

   /// Tell the pretty printer that a morsel is materialized in the sinks
   /// and can be written out.
   /// @return true if the output is closed.
   bool markMorselDone(ExecutionContext& ctx);

   /// Set the output stream for this PrettyPrinter.
   void setOstream(std::ostream& ostream);

   /// How many rows did we produce?
   size_t num_rows = 0;

   private:
   friend class Print;

   /// The IUs we produce - in the given order.
   std::vector<const IU*> ius;
   /// The column names.
   std::vector<std::string> colnames;
   /// Row limit - the output is closed once this limit is reached.
   std::optional<size_t> limit;
   /// How many morsels did we print so far?
   size_t morsel_count = 0;
   /// The output stream into which to write the pretty-printed result.
   std::ostream* out;
};

/// Print operator. Is hooked into a PrettyPrinter that writes the
/// result rows nicely formatted into an ostream.
struct Print : public RelAlgOp {

   static std::unique_ptr<Print> build(
      std::vector<std::unique_ptr<RelAlgOp>> children_,
      std::vector<const IU*> ius,
      std::vector<std::string> colnames,
      std::string op_name_ = "",
      std::optional<size_t> limit = {});

   void decay(PipelineDAG& dag) const override;

   /// The actual backing pretty-printer.
   std::unique_ptr<PrettyPrinter> printer;

   private:
   Print(std::vector<std::unique_ptr<RelAlgOp>> children_,
         std::vector<const IU*> ius,
         std::vector<std::string> colnames,
         std::string op_name_,
         std::optional<size_t> limit);
};

}

#endif //INKFUSE_PRINT_H
