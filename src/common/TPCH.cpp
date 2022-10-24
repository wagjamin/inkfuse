#include "common/TPCH.h"
#include "algebra/Aggregation.h"
#include "algebra/ExpressionOp.h"
#include "algebra/Filter.h"
#include "algebra/Print.h"
#include "algebra/TableScan.h"
#include "common/Helpers.h"

// We are not 100% standard TPC-H schema compliant.
// - DECIMALs gets represented as double
// - VARCHARs/CHARs get represented as TEXT
//   (only exception is char(1) which gets represented as char)
namespace inkfuse::tpch {

namespace {

using Node = ExpressionOp::Node;
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

std::unique_ptr<Print> q1(const Schema& schema) {
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
      ComputeNode::Type::GreaterEqual,
      IR::DateVal::build(helpers::dateStrToInt("1998-12-01") - 90),
      filter_nodes[0].get()
      ));

   std::vector<RelAlgOpPtr> children_filter_expr;
   children_filter_expr.push_back(std::move(scan));
   auto filter_expr = ExpressionOp::build(
      std::move(children_filter_expr),
      "shipdate_filter",
      std::vector<Node*>{filter_nodes[1].get()},
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
   auto one_plus_l_tax = agg_nodes.emplace_back(std::make_unique<ComputeNode>(
      ComputeNode::Type::Add,
      IR::F8::build(1.0),
      agg_nodes.back().get())).get();

   // 3.2 Eval 1 - l_discount
   agg_nodes.push_back(std::make_unique<IURefNode>(
      filter_ref.getOutput()[getScanIndex("l_discount", cols)]));
   auto one_minus_l_discount = agg_nodes.emplace_back(std::make_unique<ComputeNode>(
      ComputeNode::Type::Subtract,
      IR::F8::build(1.0),
      agg_nodes.back().get())).get();

   // 3.3 Eval l_extendedprice * (1 - l_discount)
   agg_nodes.push_back(std::make_unique<IURefNode>(
      filter_ref.getOutput()[getScanIndex("l_extendedprice", cols)]));
   auto mult_simple = agg_nodes.emplace_back(std::make_unique<ComputeNode>(
      ComputeNode::Type::Multiply,
      std::vector<Node*>{one_minus_l_discount, agg_nodes.back().get()}
   )).get();

   // 3.4 Eval l_extendedprice * (1 - l_discount) * (1 + l_tax)
   auto mult_complex = agg_nodes.emplace_back(std::make_unique<ComputeNode>(
      ComputeNode::Type::Multiply,
      std::vector<Node*>{mult_simple, one_plus_l_tax}
   )).get();

   // 3.5 Set up the operator.
   auto agg_expr = ExpressionOp::build(
      std::move(children_agg_expr),
      "agg_expr",
      std::vector<Node*>{
         mult_simple,
         mult_complex
      },
      std::move(agg_nodes));
   auto& agg_expr_ref = *agg_expr;

   // 4. Group by l_returnflag, l_linestatus & compute aggregates.
   std::vector<RelAlgOpPtr> agg_children;
   std::vector<const IU*> group_by{
      filter_ref.getOutput()[getScanIndex("l_returnflag", cols)],
      filter_ref.getOutput()[getScanIndex("l_linestatus", cols)]};
   std::vector<AggregateFunctions::Description> aggregates{
      {*filter_ref.getOutput()[getScanIndex("l_quantity", cols)],
       AggregateFunctions::Opcode::Sum},
      {*filter_ref.getOutput()[getScanIndex("l_extendedprice", cols)],
         AggregateFunctions::Opcode::Sum},
      {*agg_expr->getOutput()[0], AggregateFunctions::Opcode::Sum},
      {*agg_expr->getOutput()[1], AggregateFunctions::Opcode::Sum},
      {*filter_ref.getOutput()[getScanIndex("l_quantity", cols)],
         AggregateFunctions::Opcode::Avg},
      {*filter_ref.getOutput()[getScanIndex("l_extendedprice", cols)],
         AggregateFunctions::Opcode::Avg},
      {*filter_ref.getOutput()[getScanIndex("l_discount", cols)],
         AggregateFunctions::Opcode::Avg},
      {*filter_ref.getOutput()[getScanIndex("l_quantity", cols)],
       AggregateFunctions::Opcode::Count}};
   agg_children.push_back(std::move(agg_expr));
   auto agg = Aggregation::build(
      std::move(agg_children),
      "agg",
      std::move(group_by),
      std::move(aggregates));

   // TODO(benjamin) ORDER BY

   // Attach the sink for printing.
   std::vector<const IU*> out_ius;
   out_ius.reserve(agg->getOutput().size());
   for (const IU* iu: agg->getOutput()) {
      out_ius.push_back(iu);
   }
   std::vector<RelAlgOpPtr> print_children;
   print_children.push_back(std::move(agg));
   std::vector<std::string> colnames = {
      "l_returnflag",
      "l_linestatus",
      "sum_qty",
      "sum_base_price",
      "sum_disc_price",
      "sum_charge",
      "avg_qty",
      "avg_price",
      "avg_disc",
      "count_order"
   };
   return Print::build(std::move(print_children),
                       std::move(out_ius), std::move(colnames));
}

std::unique_ptr<Print> q6(const Schema& schema) {
   // 1. Scan from lineitem.
   auto& rel = schema.at("lineitem");
   std::vector<std::string> cols{
      "l_extendedprice",
      "l_discount",
      "l_shipdate",
      "l_discount",
      "l_quantity"
   };
   auto scan = TableScan::build(*rel, cols, "scan");
   auto& scan_ref = *scan;

   // 2. Evaluate the filter predicate.
   std::vector<RelAlgOpPtr> children_scan;
   children_scan.push_back(std::move(scan));
   std::vector<ExpressionOp::NodePtr> pred_nodes;

   auto l_shipdate_ref = pred_nodes.emplace_back(std::make_unique<IURefNode>(
         scan_ref.getOutput()[getScanIndex("l_shipdate", cols)])).get();
   auto l_discount_ref = pred_nodes.emplace_back(std::make_unique<IURefNode>(
      scan_ref.getOutput()[getScanIndex("l_discount", cols)])).get();
   auto l_quantity_ref = pred_nodes.emplace_back(std::make_unique<IURefNode>(
      scan_ref.getOutput()[getScanIndex("l_quantity", cols)])).get();
   auto pred_1 = pred_nodes.emplace_back(std::make_unique<ComputeNode>(
      ComputeNode::Type::LessEqual,
      IR::DateVal::build(helpers::dateStrToInt("1994-01-01")),
      l_shipdate_ref)).get();
   auto pred_2 = pred_nodes.emplace_back(std::make_unique<ComputeNode>(
      ComputeNode::Type::Greater,
      IR::DateVal::build(helpers::dateStrToInt("1995-01-01")),
      l_shipdate_ref)).get();
   auto pred_3 = pred_nodes.emplace_back(std::make_unique<ComputeNode>(
      ComputeNode::Type::LessEqual,
      IR::F8::build(0.06 - 0.01001),
      l_discount_ref)).get();
   auto pred_4 = pred_nodes.emplace_back(std::make_unique<ComputeNode>(
      ComputeNode::Type::GreaterEqual,
      IR::F8::build(0.06 + 0.01001),
      l_discount_ref)).get();
   auto pred_5 = pred_nodes.emplace_back(std::make_unique<ComputeNode>(
      ComputeNode::Type::Greater,
      IR::F8::build(24),
      l_quantity_ref)).get();
   auto and_1  = pred_nodes.emplace_back(std::make_unique<ComputeNode>(
      ComputeNode::Type::And,
      std::vector<Node*>{pred_1, pred_2})).get();
   auto and_2  = pred_nodes.emplace_back(std::make_unique<ComputeNode>(
      ComputeNode::Type::And,
      std::vector<Node*>{pred_3, pred_4})).get();
   auto and_3  = pred_nodes.emplace_back(std::make_unique<ComputeNode>(
      ComputeNode::Type::And,
      std::vector<Node*>{and_1, and_2})).get();
   auto and_4 = pred_nodes.emplace_back(std::make_unique<ComputeNode>(
      ComputeNode::Type::And,
      std::vector<Node*>{and_3, pred_5})).get();

   auto expr_node = ExpressionOp::build(
      std::move(children_scan),
      "lineitem_filter",
      std::vector<Node*>{and_4},
      std::move(pred_nodes));
   auto& expr_ref = *expr_node;
   assert(expr_ref.getOutput().size() == 1);

   // 3. Filter
   std::vector<RelAlgOpPtr> children_filter;
   children_filter.push_back(std::move(expr_node));
   std::vector<const IU*> redefined {
      scan_ref.getOutput()[getScanIndex("l_extendedprice", cols)],
      scan_ref.getOutput()[getScanIndex("l_discount", cols)]
   };
   auto filter = Filter::build(std::move(children_filter), "filter", std::move(redefined), *expr_ref.getOutput()[0]);
   auto& filter_ref = *filter;
   assert(filter->getOutput().size() == 2);

   // 4. Multiply.
   std::vector<RelAlgOpPtr> children_mult;
   children_mult.push_back(std::move(filter));
   std::vector<ExpressionOp::NodePtr> mult_nodes;
   auto mult_ref_1 = mult_nodes.emplace_back(std::make_unique<IURefNode>(
      filter_ref.getOutput()[0])).get();
   auto mult_ref_2 = mult_nodes.emplace_back(std::make_unique<IURefNode>(
      filter_ref.getOutput()[1])).get();
   auto mult_node = mult_nodes.emplace_back(std::make_unique<ComputeNode>(
      ComputeNode::Type::Multiply,
      std::vector<Node*>{mult_ref_1, mult_ref_2})).get();

   auto mult_expr = ExpressionOp::build(
      std::move(children_mult),
      "mult_expr",
      std::vector<Node*>{mult_node},
      std::move(mult_nodes));
   auto& mult_ref = *mult_expr;
   assert(mult_ref.getOutput().size() == 1);

   // 5. Aggregate
   std::vector<RelAlgOpPtr> agg_children;
   agg_children.push_back(std::move(mult_expr));
   // Don't group by anything on this query.
   std::vector<const IU*> group_by{};
   std::vector<AggregateFunctions::Description> aggregates{
      {*mult_ref.getOutput()[0], AggregateFunctions::Opcode::Sum}};
   auto agg = Aggregation::build(
      std::move(agg_children),
      "agg",
      std::move(group_by),
      std::move(aggregates));

   // 6. Print
   std::vector<const IU*> out_ius {agg->getOutput()[0]};
   std::vector<std::string> colnames = { "revenue" };
   std::vector<RelAlgOpPtr> print_children;
   print_children.push_back(std::move(agg));
   return Print::build(std::move(print_children),
                       std::move(out_ius), std::move(colnames));

}

}
