#include "common/TPCH.h"
#include "algebra/Aggregation.h"
#include "algebra/ExpressionOp.h"
#include "algebra/Filter.h"
#include "algebra/TableScan.h"
#include "common/Helpers.h"

// We are not 100% standard TPC-H schema compliant.
// - DECIMALs gets represented as double
// - VARCHARs/CHARs get represented as TEXT
//   (only exception is char(1) which gets represented as char)
namespace inkfuse::tpch {

namespace {

using ComputeNode = ExpressionOp::ComputeNode;
using IURefNode = ExpressionOp::IURefNode;

// Types used throughout TPC-H.
const auto t_ui4 = IR::UnsignedInt::build(4);
const auto t_f8 = IR::Float::build(8);
const auto t_char = IR::Char::build();
const auto t_date = IR::Date::build();

size_t getScanIndex(std::string_view name, const std::vector<std::string>& cols) {
   auto elem = std::find(cols.begin(), cols.end(), name);
   return std::distance(cols.begin(), elem);
}

StoredRelationPtr tableLineitem() {
   auto rel = std::make_unique<StoredRelation>();
   rel->attachPODColumn("l_orderkey", t_ui4);
   rel->attachPODColumn("l_partkey", t_ui4);
   rel->attachPODColumn("l_suppkey", t_ui4);
   rel->attachPODColumn("l_linenumber", t_ui4);
   rel->attachPODColumn("l_quantity", t_f8);
   rel->attachPODColumn("l_extendedprice", t_f8);
   rel->attachPODColumn("l_discount", t_f8);
   rel->attachPODColumn("l_tax", t_f8);
   rel->attachPODColumn("l_returnflag", t_char);
   rel->attachPODColumn("l_linestatus", t_char);
   rel->attachPODColumn("l_shipdate", t_date);
   rel->attachPODColumn("l_commitdate", t_date);
   rel->attachPODColumn("l_receiptdate", t_date);
   rel->attachStringColumn("l_shipinstruct");
   rel->attachStringColumn("l_shipmode");
   rel->attachStringColumn("l_comment");
   return rel;
}

}

Schema getTPCHSchema() {
   Schema schema;
   schema.emplace("lineitem", tableLineitem());
   return schema;
}

RelAlgOpPtr q1(const Schema& schema) {
   // 1. Scan from lineitem.
   auto& rel = schema.at("lineitem");
   std::vector<std::string> cols{
      "l_returnflag",
      "l_linestatus",
      "l_quantity",
      "l_extendedprice",
      "l_discount",
      "l_tax",
      "l_shipdate"};
   auto scan = TableScan::build(*rel, cols, "scan");
   auto& scan_ref = *scan;

   // 2. Filter l_shipdate <= date '1998-12-01' - interval '90' day
   // 2.1 Evaluate expression.
   std::vector<ExpressionOp::NodePtr> filter_nodes;
   filter_nodes.push_back(std::make_unique<IURefNode>(
      scan_ref.getOutput()[getScanIndex("l_shipdate", cols)]));
   filter_nodes.push_back(std::make_unique<ComputeNode>(
      ComputeNode::Type::LessEqual,
      filter_nodes[0].get(),
      IR::DateVal::build(helpers::dateStrToInt("1998-12-01") - 90)));

   std::vector<RelAlgOpPtr> children_filter_expr;
   children_filter_expr.push_back(std::move(scan));
   auto filter_expr = ExpressionOp::build(
      std::move(children_filter_expr),
      "shipdate_filter",
      std::vector<ExpressionOp::Node*>{filter_nodes[1].get()},
      std::move(filter_nodes));
   auto& filter_expr_ref = *filter_expr;
   assert(filter_expr_ref.getOutput().size() == 1);

   // 2.2. Filter on the expression. Need everything apart from l_shipdate.
   std::vector<RelAlgOpPtr> children_filter;
   children_filter.push_back(std::move(filter_expr));
   std::vector<const IU*> redefined = scan_ref.getOutput();
   // Get rid of l_shipdate.
   redefined.pop_back();
   auto filter = Filter::build(std::move(children_filter), "filter", std::move(redefined), *filter_expr_ref.getOutput()[0]);
   auto& filter_ref = *filter;

   // 3. Evaluate expressions for the aggregations.
   std::vector<RelAlgOpPtr> children_agg_expr;
   children_agg_expr.push_back(std::move(filter));
   std::vector<ExpressionOp::NodePtr> agg_nodes;
   // 3.1 Eval 1 + l_tax
   agg_nodes.push_back(std::make_unique<IURefNode>(
      filter_ref.getOutput()[getScanIndex("l_tax", cols)]));
   agg_nodes.push_back(std::make_unique<ComputeNode>(
      ComputeNode::Type::Add,
      agg_nodes[0].get(),
      IR::F8::build(1.0)));

   // 3.2 Eval 1 - l_discount
   // 3.3 Eval l_extendedprice * (1 - l_discount)
   // 3.4 Eval l_extendedprice * (1 - l_discount) * (1 + l_tax)
   // 3.5 Set up the operator.
   auto agg_expr = ExpressionOp::build(
      std::move(children_agg_expr),
      "agg_expr",
      std::vector<ExpressionOp::Node*>{agg_nodes[1].get()},
      std::move(agg_nodes));
   auto& agg_expr_ref = *agg_expr;

   // 4. Group by l_returnflag, l_linestatus & compute aggregates.
   std::vector<RelAlgOpPtr> agg_children;
   std::vector<const IU*> group_by{
      filter_ref.getOutput()[getScanIndex("l_returnflag", cols)],
      filter_ref.getOutput()[getScanIndex("l_linestatus", cols)]};
   std::vector<AggregateFunctions::Description> aggregates{
      {*filter_ref.getOutput()[getScanIndex("l_quantity", cols)],
       AggregateFunctions::Opcode::Count}};
   agg_children.push_back(std::move(agg_expr));
   auto agg = Aggregation::build(
      std::move(agg_children),
      "agg",
      std::move(group_by),
      std::move(aggregates));

   // TODO(benjamin) ORDER BY
   return agg;
}

}