#include "common/TPCH.h"
#include "algebra/Aggregation.h"
#include "algebra/ExpressionOp.h"
#include "algebra/Filter.h"
#include "algebra/Join.h"
#include "algebra/Print.h"
#include "algebra/TableScan.h"
#include "common/Helpers.h"

// We are not 100% standard TPC-H schema compliant.
// - DECIMALs gets represented as double
// - VARCHARs/CHARs get represented as TEXT
//   (only exception is char(1) which gets represented as char)
// This would not be good for a production engine, but is good enough
// as an experimental engine outlining the approach to query execution.
namespace inkfuse::tpch {

namespace {

using Node = ExpressionOp::Node;
using ComputeNode = ExpressionOp::ComputeNode;
using IURefNode = ExpressionOp::IURefNode;

// Types used throughout TPC-H.
const auto t_i4 = IR::SignedInt::build(4);
const auto t_f8 = IR::Float::build(8);
const auto t_char = IR::Char::build();
const auto t_date = IR::Date::build();

size_t getScanIndex(std::string_view name, const std::vector<std::string>& cols) {
   auto elem = std::find(cols.begin(), cols.end(), name);
   return std::distance(cols.begin(), elem);
}

StoredRelationPtr tableLineitem() {
   auto rel = std::make_unique<StoredRelation>();
   rel->attachPODColumn("l_orderkey", t_i4);
   rel->attachPODColumn("l_partkey", t_i4);
   rel->attachPODColumn("l_suppkey", t_i4);
   rel->attachPODColumn("l_linenumber", t_i4);
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

StoredRelationPtr tableOrders() {
   auto rel = std::make_unique<StoredRelation>();
   rel->attachPODColumn("o_orderkey", t_i4);
   rel->attachPODColumn("o_custkey", t_i4);
   rel->attachPODColumn("o_orderstatus", t_char);
   rel->attachPODColumn("o_totalprice", t_f8);
   rel->attachPODColumn("o_orderdate", t_date);
   rel->attachStringColumn("o_orderpriority");
   rel->attachStringColumn("o_clerk");
   rel->attachPODColumn("o_shippriority", t_i4);
   rel->attachStringColumn("o_comment");
   return rel;
}

StoredRelationPtr tableCustomer() {
   auto rel = std::make_unique<StoredRelation>();
   rel->attachPODColumn("c_custkey", t_i4);
   rel->attachStringColumn("c_name");
   rel->attachStringColumn("c_address");
   rel->attachPODColumn("c_nationkey", t_i4);
   rel->attachStringColumn("c_phone");
   rel->attachPODColumn("c_acctbal", t_f8);
   rel->attachStringColumn("c_mktsegment");
   rel->attachStringColumn("c_comment");
   return rel;
}

StoredRelationPtr tablePart() {
   auto rel = std::make_unique<StoredRelation>();
   rel->attachPODColumn("p_partkey", t_i4);
   rel->attachStringColumn("p_name");
   rel->attachStringColumn("p_mfgr");
   rel->attachStringColumn("p_brand");
   rel->attachStringColumn("p_type");
   rel->attachPODColumn("p_size", t_i4);
   rel->attachStringColumn("p_container");
   rel->attachPODColumn("p_retailprice", t_f8);
   rel->attachStringColumn("p_comment");
   return rel;
}

}

Schema getTPCHSchema() {
   Schema schema;
   schema.emplace("lineitem", tableLineitem());
   schema.emplace("orders", tableOrders());
   schema.emplace("customer", tableCustomer());
   schema.emplace("part", tablePart());
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
   auto scan = TableScan::build(*rel, cols, "l_scan");
   auto& scan_ref = *scan;

   // 2. Filter l_shipdate <= date '1998-12-01' - interval '90' day
   // 2.1 Evaluate expression.
   std::vector<ExpressionOp::NodePtr> filter_nodes;
   filter_nodes.push_back(std::make_unique<IURefNode>(
      scan_ref.getOutput()[getScanIndex("l_shipdate", cols)]));
   filter_nodes.push_back(std::make_unique<ComputeNode>(
      ComputeNode::Type::GreaterEqual,
      IR::DateVal::build(helpers::dateStrToInt("1998-12-01") - 90),
      filter_nodes[0].get()));

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
                                                   agg_nodes.back().get()))
                            .get();

   // 3.2 Eval 1 - l_discount
   agg_nodes.push_back(std::make_unique<IURefNode>(
      filter_ref.getOutput()[getScanIndex("l_discount", cols)]));
   auto one_minus_l_discount = agg_nodes.emplace_back(std::make_unique<ComputeNode>(
                                                         ComputeNode::Type::Subtract,
                                                         IR::F8::build(1.0),
                                                         agg_nodes.back().get()))
                                  .get();

   // 3.3 Eval l_extendedprice * (1 - l_discount)
   agg_nodes.push_back(std::make_unique<IURefNode>(
      filter_ref.getOutput()[getScanIndex("l_extendedprice", cols)]));
   auto mult_simple = agg_nodes.emplace_back(std::make_unique<ComputeNode>(
                                                ComputeNode::Type::Multiply,
                                                std::vector<Node*>{one_minus_l_discount, agg_nodes.back().get()}))
                         .get();

   // 3.4 Eval l_extendedprice * (1 - l_discount) * (1 + l_tax)
   auto mult_complex = agg_nodes.emplace_back(std::make_unique<ComputeNode>(
                                                 ComputeNode::Type::Multiply,
                                                 std::vector<Node*>{mult_simple, one_plus_l_tax}))
                          .get();

   // 3.5 Set up the operator.
   auto agg_expr = ExpressionOp::build(
      std::move(children_agg_expr),
      "agg_expr",
      std::vector<Node*>{
         mult_simple,
         mult_complex},
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
   for (const IU* iu : agg->getOutput()) {
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
      "count_order"};
   return Print::build(std::move(print_children),
                       std::move(out_ius), std::move(colnames));
}

std::unique_ptr<Print> q3(const Schema& schema) {
   // 1. Join Customer <-> Orders
   // 1.1 Scan customer
   auto& c_rel = schema.at("customer");
   std::vector<std::string> c_cols{
      "c_custkey",
      "c_mktsegment",
   };
   auto c_scan = TableScan::build(*c_rel, c_cols, "scan");
   auto& c_scan_ref = *c_scan;

   // 1.2 Filter customer on c_mktsegment = 'BUILDING'
   std::vector<ExpressionOp::NodePtr> c_nodes;
   c_nodes.emplace_back(std::make_unique<IURefNode>(c_scan_ref.getOutput()[1]));
   c_nodes.emplace_back(std::make_unique<ComputeNode>(ComputeNode::Type::StrEquals, IR::StringVal::build("BUILDING"), c_nodes[0].get()));
   auto c_nodes_root = c_nodes[1].get();
   std::vector<RelAlgOpPtr> c_expr_children;
   c_expr_children.push_back(std::move(c_scan));
   auto c_expr = ExpressionOp::build(
      std::move(c_expr_children),
      "expr_customer",
      {c_nodes_root},
      std::move(c_nodes));
   auto& c_expr_ref = *c_expr;

   std::vector<RelAlgOpPtr> c_filter_children;
   c_filter_children.push_back(std::move(c_expr));
   auto c_filter = Filter::build(
      std::move(c_filter_children),
      "filter_customer",
      {c_scan_ref.getOutput()[0]},
      *c_expr_ref.getOutput()[0]);
   auto& c_filter_ref = *c_filter;

   // 1.2 Scan orders
   auto& o_rel = schema.at("orders");
   std::vector<std::string> o_cols{
      "o_orderkey",
      "o_custkey",
      "o_orderdate",
      "o_shippriority",
   };
   auto o_scan = TableScan::build(*o_rel, o_cols, "scan");
   auto& o_scan_ref = *o_scan;
   // 1.3 Filter orders on o_orderdate < '1995-03-15'
   std::vector<ExpressionOp::NodePtr> o_nodes;
   o_nodes.emplace_back(std::make_unique<IURefNode>(o_scan_ref.getOutput()[2]));
   o_nodes.emplace_back(
      std::make_unique<ComputeNode>(
         ComputeNode::Type::Greater,
         IR::DateVal::build(helpers::dateStrToInt("1995-03-15")),
         o_nodes[0].get()));
   auto o_nodes_root = o_nodes[1].get();
   std::vector<RelAlgOpPtr> o_expr_children;
   o_expr_children.push_back(std::move(o_scan));
   auto o_expr = ExpressionOp::build(
      std::move(o_expr_children),
      "expr_orders",
      {o_nodes_root},
      std::move(o_nodes));
   auto& o_expr_ref = *o_expr;

   std::vector<RelAlgOpPtr> o_filter_children;
   o_filter_children.push_back(std::move(o_expr));
   auto o_filter = Filter::build(
      std::move(o_filter_children),
      "filter_orders",
      // We will need all output columns again.
      {
         o_scan_ref.getOutput()[0],
         o_scan_ref.getOutput()[1],
         o_scan_ref.getOutput()[2],
         o_scan_ref.getOutput()[3],
      },
      *o_expr_ref.getOutput()[0]);
   auto& o_filter_ref = *o_filter;

   // 1.4 Join the two tables.
   // The engine can infer that this is a PK join, c_custkey is PK of customer
   std::vector<RelAlgOpPtr> c_o_join_children;
   c_o_join_children.push_back(std::move(c_filter));
   c_o_join_children.push_back(std::move(o_filter));
   auto c_o_join = Join::build(
      std::move(c_o_join_children),
      "c_o_join",
      // Keys left (c_custkey)
      {c_filter_ref.getOutput()[0]},
      // Payload left ()
      {},
      // Keys right (o_custkey)
      {o_filter_ref.getOutput()[1]},
      // Payload right (o_orderkey, o_orderdate, o_shippriority)
      {
         o_filter_ref.getOutput()[0],
         o_filter_ref.getOutput()[2],
         o_filter_ref.getOutput()[3],
      },
      JoinType::Inner,
      true);
   auto& c_o_join_ref = *c_o_join;

   // 2. Join This on Lineitem
   // 2.1 Scan lineitem
   auto& l_rel = schema.at("lineitem");
   std::vector<std::string> l_cols{
      "l_orderkey",
      "l_discount",
      "l_shipdate",
      "l_extendedprice"};
   auto l_scan = TableScan::build(*l_rel, l_cols, "l_scan");
   auto& l_scan_ref = *l_scan;

   // 2.2 Filter lineitem on l_shipdate > '1995-03-15'
   std::vector<ExpressionOp::NodePtr> l_nodes;
   l_nodes.emplace_back(std::make_unique<IURefNode>(l_scan_ref.getOutput()[2]));
   l_nodes.emplace_back(std::make_unique<ComputeNode>(ComputeNode::Type::Less, IR::DateVal::build(helpers::dateStrToInt("1995-03-15")), l_nodes[0].get()));
   auto l_nodes_root = l_nodes[1].get();
   std::vector<RelAlgOpPtr> l_expr_children;
   l_expr_children.push_back(std::move(l_scan));
   auto l_expr = ExpressionOp::build(
      std::move(l_expr_children),
      "expr_lineitem",
      {l_nodes_root},
      std::move(l_nodes));
   auto& l_expr_ref = *l_expr;

   std::vector<RelAlgOpPtr> l_filter_children;
   l_filter_children.push_back(std::move(l_expr));
   auto l_filter = Filter::build(
      std::move(l_filter_children),
      "filter_lineitem",
      // We will need all output columns apart from l_shipdate.
      {
         l_scan_ref.getOutput()[0],
         l_scan_ref.getOutput()[1],
         l_scan_ref.getOutput()[3],
      },
      *l_expr_ref.getOutput()[0]);
   auto& l_filter_ref = *l_filter;

   // 2.3 Perform the actual join
   // The engine can infer that this is a PK join, o_custkey is PK of orders
   // And initial join was a PK join (i.e. at most one match per row)
   std::vector<RelAlgOpPtr> c_o_l_join_children;
   c_o_l_join_children.push_back(std::move(c_o_join));
   c_o_l_join_children.push_back(std::move(l_filter));
   auto c_o_l_join = Join::build(
      std::move(c_o_l_join_children),
      "c_o_l_join",
      // Keys left (o_orderkey)
      {c_o_join_ref.getOutput()[2]},
      // Payload left (o_orderdate, o_shippriority)
      {
         c_o_join_ref.getOutput()[3],
         c_o_join_ref.getOutput()[4],
      },
      // Keys right (l_orderkey)
      {l_filter_ref.getOutput()[0]},
      // Payload right (l_discount, l_extendedprice)
      {
         l_filter_ref.getOutput()[1],
         l_filter_ref.getOutput()[2],
      },
      JoinType::Inner,
      true);
   auto& c_o_l_join_ref = *c_o_l_join;

   // 3. Aggregate
   // 3.1 Calculate `l_extendedprice * (1 - l_discount)`
   std::vector<ExpressionOp::NodePtr> agg_nodes;
   agg_nodes.emplace_back(std::make_unique<IURefNode>(c_o_l_join_ref.getOutput()[4]));
   agg_nodes.emplace_back(std::make_unique<IURefNode>(c_o_l_join_ref.getOutput()[5]));
   agg_nodes.emplace_back(std::make_unique<ComputeNode>(ComputeNode::Type::Subtract, IR::F8::build(1.0), agg_nodes[0].get()));
   agg_nodes.emplace_back(std::make_unique<ComputeNode>(ComputeNode::Type::Multiply, std::vector<Node*>{agg_nodes[1].get(), agg_nodes[2].get()}));
   auto agg_nodes_root = agg_nodes[3].get();
   std::vector<RelAlgOpPtr> agg_e_children;
   agg_e_children.push_back(std::move(c_o_l_join));
   auto agg_e = ExpressionOp::build(
      std::move(agg_e_children),
      "agg_expr",
      {agg_nodes_root},
      std::move(agg_nodes));
   auto& agg_e_ref = *agg_e;

   // 3.2 Perform the aggregation.
   std::vector<RelAlgOpPtr> agg_children;
   agg_children.push_back(std::move(agg_e));
   // Group by (l_orderkey, o_orderdate, o_shippriority).
   std::vector<const IU*> group_by{
      c_o_l_join_ref.getOutput()[0],
      c_o_l_join_ref.getOutput()[1],
      c_o_l_join_ref.getOutput()[2],
   };
   std::vector<AggregateFunctions::Description> aggregates{
      {*agg_e_ref.getOutput()[0], AggregateFunctions::Opcode::Sum}};
   auto agg = Aggregation::build(
      std::move(agg_children),
      "agg",
      std::move(group_by),
      std::move(aggregates));

   // 4. Print
   std::vector<const IU*> out_ius{
      agg->getOutput()[0],
      agg->getOutput()[3],
      agg->getOutput()[1],
      agg->getOutput()[2],
   };
   std::vector<std::string> colnames = {"l_orderkey", "revenue", "o_orderdate", "o_shippriority"};
   std::vector<RelAlgOpPtr> print_children;
   print_children.push_back(std::move(agg));
   return Print::build(std::move(print_children),
                       std::move(out_ius), std::move(colnames), "q3_print", 10);
}

std::unique_ptr<Print> q4(const Schema& schema) {
   // 1. Scan from orders.
   // 1.1 Set up the scan.
   auto& o_rel = schema.at("orders");
   std::vector<std::string> o_cols{
      "o_orderkey",
      "o_orderdate",
      "o_orderpriority",
   };
   auto o_scan = TableScan::build(*o_rel, o_cols, "scan");
   auto& o_scan_ref = *o_scan;
   // 1.2 Filter orders on o_orderdate >= '1993-07-01' and < '1993-10-01'
   std::vector<ExpressionOp::NodePtr> o_nodes;
   o_nodes.emplace_back(std::make_unique<IURefNode>(o_scan_ref.getOutput()[1]));
   o_nodes.emplace_back(
      std::make_unique<ComputeNode>(
         ComputeNode::Type::LessEqual,
         IR::DateVal::build(helpers::dateStrToInt("1993-07-01")),
         o_nodes[0].get()));
   o_nodes.emplace_back(
      std::make_unique<ComputeNode>(
         ComputeNode::Type::Greater,
         IR::DateVal::build(helpers::dateStrToInt("1993-10-01")),
         o_nodes[0].get()));
   o_nodes.emplace_back(
      std::make_unique<ComputeNode>(
         ComputeNode::Type::And,
         std::vector<Node*>{o_nodes[1].get(), o_nodes[2].get()}));
   auto o_nodes_root = o_nodes[3].get();
   std::vector<RelAlgOpPtr> o_expr_children;
   o_expr_children.push_back(std::move(o_scan));
   auto o_expr = ExpressionOp::build(
      std::move(o_expr_children),
      "expr_orders",
      {o_nodes_root},
      std::move(o_nodes));
   auto& o_expr_ref = *o_expr;

   std::vector<RelAlgOpPtr> o_filter_children;
   o_filter_children.push_back(std::move(o_expr));
   auto o_filter = Filter::build(
      std::move(o_filter_children),
      "filter_customer",
      // We need (o_orderkey, o_orderpriroity)
      {
         o_scan_ref.getOutput()[0],
         o_scan_ref.getOutput()[2],
      },
      *o_expr_ref.getOutput()[0]);
   auto& o_filter_ref = *o_filter;

   // 2. Scan from lineitem.
   // 2.1 Set up the scan
   auto& l_rel = schema.at("lineitem");
   std::vector<std::string> l_cols{
      "l_orderkey",
      "l_commitdate",
      "l_receiptdate",
   };
   auto l_scan = TableScan::build(*l_rel, l_cols, "l_scan");
   auto& l_scan_ref = *l_scan;

   // 2.2 Filter lineitem on l_commitdate < l_receiptdate
   std::vector<ExpressionOp::NodePtr> l_nodes;
   l_nodes.emplace_back(std::make_unique<IURefNode>(l_scan_ref.getOutput()[1]));
   l_nodes.emplace_back(std::make_unique<IURefNode>(l_scan_ref.getOutput()[2]));
   l_nodes.emplace_back(std::make_unique<ComputeNode>(ComputeNode::Type::Less, std::vector<Node*>{l_nodes[0].get(), l_nodes[1].get()}));
   auto l_nodes_root = l_nodes[2].get();
   std::vector<RelAlgOpPtr> l_expr_children;
   l_expr_children.push_back(std::move(l_scan));
   auto l_expr = ExpressionOp::build(
      std::move(l_expr_children),
      "expr_lineitem",
      {l_nodes_root},
      std::move(l_nodes));
   auto& l_expr_ref = *l_expr;

   std::vector<RelAlgOpPtr> l_filter_children;
   l_filter_children.push_back(std::move(l_expr));
   auto l_filter = Filter::build(
      std::move(l_filter_children),
      "filter_lineitem",
      // We only need l_orderkey.
      {
         l_scan_ref.getOutput()[0],
      },
      *l_expr_ref.getOutput()[0]);
   auto& l_filter_ref = *l_filter;

   // 3. Join the two. The engine can infer it's a PK join as o_orderkey is PK of orders.
   std::vector<RelAlgOpPtr> o_l_join_children;
   o_l_join_children.push_back(std::move(o_filter));
   o_l_join_children.push_back(std::move(l_filter));
   auto o_l_join = Join::build(
      std::move(o_l_join_children),
      "o_l_join",
      // Keys left (o_orderkey)
      {o_filter_ref.getOutput()[0]},
      // Payload left (o_orderpriority)
      {
         o_filter_ref.getOutput()[1],
      },
      // Keys right (l_orderkey)
      {l_filter_ref.getOutput()[0]},
      // No right payload - semi join.
      {},
      JoinType::LeftSemi,
      true);
   auto& o_l_join_ref = *o_l_join;
   assert(o_l_join_ref.getOutput().size() == 2);

   // 4. Aggregate.
   std::vector<RelAlgOpPtr> agg_children;
   agg_children.push_back(std::move(o_l_join));
   // Group by (o_orderpriority).
   std::vector<const IU*> group_by{
      o_l_join_ref.getOutput()[1],
   };
   std::vector<AggregateFunctions::Description> aggregates{
      {*o_l_join_ref.getOutput()[1], AggregateFunctions::Opcode::Count}};
   auto agg = Aggregation::build(
      std::move(agg_children),
      "agg",
      std::move(group_by),
      std::move(aggregates));

   // 5. Print.
   std::vector<const IU*> out_ius{
      agg->getOutput()[0],
      agg->getOutput()[1],
   };
   std::vector<std::string> colnames = {"o_orderpriority", "order_count"};
   std::vector<RelAlgOpPtr> print_children;
   print_children.push_back(std::move(agg));
   return Print::build(std::move(print_children),
                       std::move(out_ius), std::move(colnames), "print");
}

std::unique_ptr<Print> q6(const Schema& schema) {
   // 1. Scan from lineitem.
   auto& rel = schema.at("lineitem");
   std::vector<std::string> cols{
      "l_extendedprice",
      "l_discount",
      "l_shipdate",
      "l_quantity"};
   auto scan = TableScan::build(*rel, cols, "scan");
   auto& scan_ref = *scan;

   // 2. Evaluate the filter predicate.
   std::vector<RelAlgOpPtr> children_scan;
   children_scan.push_back(std::move(scan));
   std::vector<ExpressionOp::NodePtr> pred_nodes;

   auto l_shipdate_ref = pred_nodes.emplace_back(std::make_unique<IURefNode>(
                                                    scan_ref.getOutput()[getScanIndex("l_shipdate", cols)]))
                            .get();
   auto l_discount_ref = pred_nodes.emplace_back(std::make_unique<IURefNode>(
                                                    scan_ref.getOutput()[getScanIndex("l_discount", cols)]))
                            .get();
   auto l_quantity_ref = pred_nodes.emplace_back(std::make_unique<IURefNode>(
                                                    scan_ref.getOutput()[getScanIndex("l_quantity", cols)]))
                            .get();
   auto pred_1 = pred_nodes.emplace_back(std::make_unique<ComputeNode>(
                                            ComputeNode::Type::LessEqual,
                                            IR::DateVal::build(helpers::dateStrToInt("1994-01-01")),
                                            l_shipdate_ref))
                    .get();
   auto pred_2 = pred_nodes.emplace_back(std::make_unique<ComputeNode>(
                                            ComputeNode::Type::Greater,
                                            IR::DateVal::build(helpers::dateStrToInt("1995-01-01")),
                                            l_shipdate_ref))
                    .get();
   // We have some rounding issues if we don't use 0.01001.
   auto pred_3 = pred_nodes.emplace_back(std::make_unique<ComputeNode>(
                                            ComputeNode::Type::LessEqual,
                                            IR::F8::build(0.06 - 0.01001),
                                            l_discount_ref))
                    .get();
   auto pred_4 = pred_nodes.emplace_back(std::make_unique<ComputeNode>(
                                            ComputeNode::Type::GreaterEqual,
                                            IR::F8::build(0.06 + 0.01001),
                                            l_discount_ref))
                    .get();
   auto pred_5 = pred_nodes.emplace_back(std::make_unique<ComputeNode>(
                                            ComputeNode::Type::Greater,
                                            IR::F8::build(24),
                                            l_quantity_ref))
                    .get();
   auto and_1 = pred_nodes.emplace_back(std::make_unique<ComputeNode>(
                                           ComputeNode::Type::And,
                                           std::vector<Node*>{pred_1, pred_2}))
                   .get();
   auto and_2 = pred_nodes.emplace_back(std::make_unique<ComputeNode>(
                                           ComputeNode::Type::And,
                                           std::vector<Node*>{pred_3, pred_4}))
                   .get();
   auto and_3 = pred_nodes.emplace_back(std::make_unique<ComputeNode>(
                                           ComputeNode::Type::And,
                                           std::vector<Node*>{and_1, and_2}))
                   .get();
   auto and_4 = pred_nodes.emplace_back(std::make_unique<ComputeNode>(
                                           ComputeNode::Type::And,
                                           std::vector<Node*>{and_3, pred_5}))
                   .get();

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
   std::vector<const IU*> redefined{
      scan_ref.getOutput()[getScanIndex("l_extendedprice", cols)],
      scan_ref.getOutput()[getScanIndex("l_discount", cols)]};
   auto filter = Filter::build(std::move(children_filter), "filter", std::move(redefined), *expr_ref.getOutput()[0]);
   auto& filter_ref = *filter;
   assert(filter->getOutput().size() == 2);

   // 4. Multiply.
   std::vector<RelAlgOpPtr> children_mult;
   children_mult.push_back(std::move(filter));
   std::vector<ExpressionOp::NodePtr> mult_nodes;
   auto mult_ref_1 = mult_nodes.emplace_back(std::make_unique<IURefNode>(
                                                filter_ref.getOutput()[0]))
                        .get();
   auto mult_ref_2 = mult_nodes.emplace_back(std::make_unique<IURefNode>(
                                                filter_ref.getOutput()[1]))
                        .get();
   auto mult_node = mult_nodes.emplace_back(std::make_unique<ComputeNode>(
                                               ComputeNode::Type::Multiply,
                                               std::vector<Node*>{mult_ref_1, mult_ref_2}))
                       .get();

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
   std::vector<const IU*> out_ius{agg->getOutput()[0]};
   std::vector<std::string> colnames = {"revenue"};
   std::vector<RelAlgOpPtr> print_children;
   print_children.push_back(std::move(agg));
   return Print::build(std::move(print_children),
                       std::move(out_ius), std::move(colnames));
}

// No support for case when - only aggregate
// sum(l_extendedprice * (1 - l_discount)) as promo_revenue
// Note that we swap build + probe side compared to DuckDB/Umbra.
// That way the join becomes a PK join.
std::unique_ptr<Print> q14(const Schema& schema) {
   // 1. Scan from part.
   auto& p_rel = schema.at("part");
   std::vector<std::string> p_cols{"p_partkey"};
   auto p_scan = TableScan::build(*p_rel, p_cols, "p_scan");
   auto& p_scan_ref = *p_scan;

   // 2. Scan from lineitem with a filter.
   auto& l_rel = schema.at("lineitem");
   std::vector<std::string> l_cols{
      "l_partkey",
      "l_shipdate",
      "l_extendedprice",
      "l_discount",
   };
   auto l_scan = TableScan::build(*l_rel, l_cols, "l_scan");
   auto& l_scan_ref = *l_scan;
   // 1.2 Filter orders on l_oshipdate >= '1993-07-01' and < '1993-10-01'
   std::vector<ExpressionOp::NodePtr> l_nodes;
   l_nodes.emplace_back(std::make_unique<IURefNode>(l_scan_ref.getOutput()[1]));
   l_nodes.emplace_back(
      std::make_unique<ComputeNode>(
         ComputeNode::Type::LessEqual,
         IR::DateVal::build(helpers::dateStrToInt("1995-09-01")),
         l_nodes[0].get()));
   l_nodes.emplace_back(
      std::make_unique<ComputeNode>(
         ComputeNode::Type::Greater,
         IR::DateVal::build(helpers::dateStrToInt("1995-10-01")),
         l_nodes[0].get()));
   l_nodes.emplace_back(
      std::make_unique<ComputeNode>(
         ComputeNode::Type::And,
         std::vector<Node*>{l_nodes[1].get(), l_nodes[2].get()}));
   auto l_nodes_root = l_nodes[3].get();
   std::vector<RelAlgOpPtr> l_expr_children;
   l_expr_children.push_back(std::move(l_scan));
   auto l_expr = ExpressionOp::build(
      std::move(l_expr_children),
      "expr_lineitem",
      {l_nodes_root},
      std::move(l_nodes));
   auto& l_expr_ref = *l_expr;

   std::vector<RelAlgOpPtr> l_filter_children;
   l_filter_children.push_back(std::move(l_expr));
   auto l_filter = Filter::build(
      std::move(l_filter_children),
      "filter_lineitem",
      // We need (l_partkey, l_extendedprice, l_discount)
      {
         l_scan_ref.getOutput()[0],
         l_scan_ref.getOutput()[2],
         l_scan_ref.getOutput()[3],
      },
      *l_expr_ref.getOutput()[0]);
   auto& l_filter_ref = *l_filter;

   // 3. Join
   std::vector<RelAlgOpPtr> p_l_join_children;
   p_l_join_children.push_back(std::move(p_scan));
   p_l_join_children.push_back(std::move(l_filter));
   auto p_l_join = Join::build(
      std::move(p_l_join_children),
      "p_l_join",
      // Keys left (p_partkey)
      {p_scan_ref.getOutput()[0]},
      // Payload left (none)
      {},
      // Keys right (l_partkey)
      {l_filter_ref.getOutput()[0]},
      // Right payload (l_extendedprice, l_discount)
      {
         l_filter_ref.getOutput()[1],
         l_filter_ref.getOutput()[2],
      },
      JoinType::Inner,
      true);
   auto& p_l_join_ref = *p_l_join;
   assert(p_l_join_ref.getOutput().size() == 4);

   // 4. Project and Aggregate
   // 4.1 Project l_extendedprice * (1 - l_discount)
   std::vector<ExpressionOp::NodePtr> pr_nodes;
   pr_nodes.emplace_back(std::make_unique<IURefNode>(p_l_join_ref.getOutput()[2]));
   pr_nodes.emplace_back(std::make_unique<IURefNode>(p_l_join_ref.getOutput()[3]));
   pr_nodes.emplace_back(
      std::make_unique<ComputeNode>(
         ComputeNode::Type::Subtract,
         IR::F8::build(1.0),
         pr_nodes[1].get()));
   pr_nodes.emplace_back(
      std::make_unique<ComputeNode>(
         ComputeNode::Type::And,
         std::vector<Node*>{pr_nodes[1].get(), pr_nodes[2].get()}));
   pr_nodes.emplace_back(
      std::make_unique<ComputeNode>(
         ComputeNode::Type::Multiply,
         std::vector<ExpressionOp::Node*>{pr_nodes[0].get(),
                                          pr_nodes[2].get()}));
   auto pr_nodes_root = pr_nodes[4].get();
   std::vector<RelAlgOpPtr> pr_expr_children;
   pr_expr_children.push_back(std::move(p_l_join));
   auto pr_expr = ExpressionOp::build(
      std::move(pr_expr_children),
      "pre_agg_project",
      {pr_nodes_root},
      std::move(pr_nodes));
   auto& pr_expr_ref = *pr_expr;

   // 4.2 Aggregate sum(l_extendedprice * (1 - l_discount))
   std::vector<RelAlgOpPtr> agg_children;
   agg_children.push_back(std::move(pr_expr));
   // Don't group by anything on this query.
   std::vector<const IU*> group_by{};
   std::vector<AggregateFunctions::Description> aggregates{
      {*pr_expr_ref.getOutput()[0], AggregateFunctions::Opcode::Sum}};
   auto agg = Aggregation::build(
      std::move(agg_children),
      "agg",
      std::move(group_by),
      std::move(aggregates));

   // 5. Print
   std::vector<const IU*> out_ius{agg->getOutput()[0]};
   std::vector<std::string> colnames = {"pomo_revenue"};
   std::vector<RelAlgOpPtr> print_children;
   print_children.push_back(std::move(agg));
   return Print::build(std::move(print_children),
                       std::move(out_ius), std::move(colnames));
}

std::unique_ptr<Print> q18(const Schema& schema) {
   // 1. Scan Lineitem & Aggregate
   // 1.1 Scan from lineitem.
   auto& l_1_rel = schema.at("lineitem");
   std::vector<std::string> l_1_cols{"l_orderkey", "l_quantity"};
   auto l_1_scan = TableScan::build(*l_1_rel, l_1_cols, "scan_lineitem_1");
   auto& l_1_scan_ref = *l_1_scan;

   // 1.2 Aggregate.
   std::vector<RelAlgOpPtr> l_agg_children;
   l_agg_children.push_back(std::move(l_1_scan));
   // Group by l_orderkey
   std::vector<const IU*> l_group_by{l_1_scan_ref.getOutput()[0]};
   std::vector<AggregateFunctions::Description> aggregates{
      {*l_1_scan_ref.getOutput()[1], AggregateFunctions::Opcode::Sum}};
   auto agg = Aggregation::build(
      std::move(l_agg_children),
      "l_agg",
      std::move(l_group_by),
      std::move(aggregates));
   auto& agg_ref = *agg;

   // 2. Filter sum(l_quantity) > 300
   std::vector<RelAlgOpPtr> expr_children;
   expr_children.push_back(std::move(agg));
   std::vector<ExpressionOp::NodePtr> pred_nodes;
   pred_nodes.emplace_back(std::make_unique<IURefNode>(agg_ref.getOutput()[1]));
   pred_nodes.emplace_back(
      std::make_unique<ComputeNode>(
         ComputeNode::Type::Less,
         IR::F8::build(300.0),
         pred_nodes[0].get()));
   auto filter_col = pred_nodes[1].get();
   auto expr_node = ExpressionOp::build(
      std::move(expr_children),
      "sum_filter",
      std::vector<Node*>{filter_col},
      std::move(pred_nodes));
   auto& expr_ref = *expr_node;
   assert(expr_ref.getOutput().size() == 1);

   std::vector<RelAlgOpPtr> filter_children;
   filter_children.push_back(std::move(expr_node));
   std::vector<const IU*> redefined{
      agg_ref.getOutput()[0]};
   auto filter = Filter::build(std::move(filter_children), "filter", std::move(redefined), *expr_ref.getOutput()[0]);
   auto& filter_ref = *filter;
   assert(filter->getOutput().size() == 1);

   // 3. Join on orders.
   auto& o_rel = schema.at("orders");
   std::vector<std::string> o_cols{"o_orderkey", "o_custkey"};
   auto o_scan = TableScan::build(*o_rel, o_cols, "scan_orders");
   auto& o_scan_ref = *o_scan;

   std::vector<RelAlgOpPtr> l_o_join_children;
   l_o_join_children.push_back(std::move(filter));
   l_o_join_children.push_back(std::move(o_scan));
   auto l_o_join = Join::build(
      std::move(l_o_join_children),
      "l_o_join",
      // Keys left (l_orderkey)
      {filter_ref.getOutput()[0]},
      // Payload left (none)
      {},
      // Keys right (o_orderkey)
      {o_scan_ref.getOutput()[0]},
      // Right payload (o_custkey)
      {
         o_scan_ref.getOutput()[1],
      },
      JoinType::Inner,
      true);
   auto& l_o_join_ref = *l_o_join;
   assert(l_o_join_ref.getOutput().size() == 3);

   // 4. Join on customer.
   auto& c_rel = schema.at("customer");
   std::vector<std::string> c_cols{"c_custkey"};
   auto c_scan = TableScan::build(*c_rel, c_cols, "scan_customer");
   auto& c_scan_ref = *c_scan;

   std::vector<RelAlgOpPtr> l_o_c_join_children;
   l_o_c_join_children.push_back(std::move(c_scan));
   l_o_c_join_children.push_back(std::move(l_o_join));
   auto l_o_c_join = Join::build(
      std::move(l_o_c_join_children),
      "l_o_c_join",
      // Keys left (c_custkey)
      {c_scan_ref.getOutput()[0]},
      // Payload left (none)
      {},
      // Keys right (o_custkey)
      {l_o_join_ref.getOutput()[2]},
      // Right payload (o_orderkey)
      {
         l_o_join_ref.getOutput()[1],
      },
      JoinType::Inner,
      true);
   auto& l_o_c_join_ref = *l_o_c_join;
   assert(l_o_c_join_ref.getOutput().size() == 3);

   // 5. Join back on lineitem.
   std::vector<std::string> l_2_cols{"l_orderkey", "l_quantity"};
   auto l_2_scan = TableScan::build(*l_1_rel, l_2_cols, "scan_lineitem_2");
   auto& l_2_scan_ref = *l_2_scan;

   std::vector<RelAlgOpPtr> l_o_c_l_join_children;
   l_o_c_l_join_children.push_back(std::move(l_o_c_join));
   l_o_c_l_join_children.push_back(std::move(l_2_scan));
   auto l_o_c_l_join = Join::build(
      std::move(l_o_c_l_join_children),
      "l_o_c_l_join",
      // Keys left (o_orderkey)
      {l_o_c_join_ref.getOutput()[2]},
      // Payload left (none)
      {},
      // Keys right (l_orderkey)
      {l_2_scan_ref.getOutput()[0]},
      // Right payload (l_quantity)
      {
         l_2_scan_ref.getOutput()[1],
      },
      JoinType::Inner,
      true);
   auto& l_o_c_l_join_ref = *l_o_c_l_join;
   assert(l_o_c_l_join_ref.getOutput().size() == 3);

   // 6. Aggregate
   std::vector<RelAlgOpPtr> agg_children;
   agg_children.push_back(std::move(l_o_c_l_join));
   // Group by l_orderkey
   std::vector<const IU*> group_by{l_o_c_l_join_ref.getOutput()[1]};
   std::vector<AggregateFunctions::Description> final_aggregates{
      {*l_o_c_l_join_ref.getOutput()[2], AggregateFunctions::Opcode::Sum}};
   auto final_agg = Aggregation::build(
      std::move(agg_children),
      "l_agg",
      std::move(group_by),
      std::move(final_aggregates));
   auto& final_agg_ref = *final_agg;

   // 4. Print
   std::vector<const IU*> out_ius{
      final_agg_ref.getOutput()[0],
      final_agg_ref.getOutput()[1],
   };
   std::vector<std::string> colnames = {"o_orderkey", "sum(l_quantity)"};
   std::vector<RelAlgOpPtr> print_children;
   print_children.push_back(std::move(final_agg));
   return Print::build(std::move(print_children),
                       std::move(out_ius), std::move(colnames));
}

std::unique_ptr<Print> l_count(const inkfuse::Schema& schema) {
   // 1. Scan from lineitem.
   auto& rel = schema.at("lineitem");
   std::vector<std::string> cols{"l_linestatus"};
   auto scan = TableScan::build(*rel, cols, "scan");
   auto& scan_ref = *scan;

   // 2. Aggregate.
   std::vector<RelAlgOpPtr> agg_children;
   agg_children.push_back(std::move(scan));
   // Don't group by anything on this query.
   std::vector<const IU*> group_by{};
   std::vector<AggregateFunctions::Description> aggregates{
      {*scan_ref.getOutput()[0], AggregateFunctions::Opcode::Count}};
   auto agg = Aggregation::build(
      std::move(agg_children),
      "agg",
      std::move(group_by),
      std::move(aggregates));

   // 6. Print
   std::vector<const IU*> out_ius{agg->getOutput()[0]};
   std::vector<std::string> colnames = {"num_rows"};
   std::vector<RelAlgOpPtr> print_children;
   print_children.push_back(std::move(agg));
   return Print::build(std::move(print_children),
                       std::move(out_ius), std::move(colnames));
}

std::unique_ptr<Print> l_point(const inkfuse::Schema& schema) {
   // 1. Scan from lineitem.
   auto& rel = schema.at("lineitem");
   std::vector<std::string> cols{"l_orderkey", "l_tax"};
   auto scan = TableScan::build(*rel, cols, "scan");
   auto& scan_ref = *scan;

   // 2. Expression & Filter
   std::vector<RelAlgOpPtr> children_scan;
   children_scan.push_back(std::move(scan));
   std::vector<ExpressionOp::NodePtr> pred_nodes;

   auto l_orderkey_ref = pred_nodes.emplace_back(std::make_unique<IURefNode>(
                                                    scan_ref.getOutput()[getScanIndex("l_orderkey", cols)]))
                            .get();
   auto pred_1 = pred_nodes.emplace_back(
                              std::make_unique<ComputeNode>(
                                 ComputeNode::Type::Eq,
                                 IR::SI<4>::build(39),
                                 l_orderkey_ref))
                    .get();
   auto expr_node = ExpressionOp::build(
      std::move(children_scan),
      "lineitem_filter",
      std::vector<Node*>{pred_1},
      std::move(pred_nodes));
   auto& expr_ref = *expr_node;
   assert(expr_ref.getOutput().size() == 1);

   // 3. Filter
   std::vector<RelAlgOpPtr> children_filter;
   children_filter.push_back(std::move(expr_node));
   std::vector<const IU*> redefined{
      scan_ref.getOutput()[getScanIndex("l_tax", cols)],
   };
   auto filter = Filter::build(std::move(children_filter), "filter", std::move(redefined), *expr_ref.getOutput()[0]);
   auto& filter_ref = *filter;
   assert(filter->getOutput().size() == 1);

   // 6. Print
   std::vector<RelAlgOpPtr> print_children;
   print_children.push_back(std::move(filter));
   std::vector<const IU*> out_ius{filter_ref.getOutput()[0]};
   std::vector<std::string> colnames = {"l_tax"};
   return Print::build(std::move(print_children),
                       std::move(out_ius), std::move(colnames));
}

}
