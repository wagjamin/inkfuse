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

// Utility to make it easier to build expressions.
template <ComputeNode::Type type>
Node* Concatenate(
   std::vector<ExpressionOp::NodePtr>& pred_nodes,
   std::vector<Node*>
      or_nodes) {
   static_assert(type == ComputeNode::Type::Or || type == ComputeNode::Type::And);
   assert(or_nodes.size() >= 2);
   Node* curr_top = pred_nodes.emplace_back(
                                 std::make_unique<ComputeNode>(
                                    type,
                                    std::vector<Node*>{or_nodes[0], or_nodes[1]}))
                       .get();
   for (auto node = or_nodes.begin() + 2; node < or_nodes.end(); ++node) {
      curr_top = pred_nodes.emplace_back(
                              std::make_unique<ComputeNode>(
                                 type,
                                 std::vector<Node*>{curr_top, *node}))
                    .get();
   }
   return curr_top;
}

constexpr auto Andify = Concatenate<ComputeNode::Type::And>;
constexpr auto Orify = Concatenate<ComputeNode::Type::Or>;

// Types used throughout TPC-H.
const auto t_i4 = IR::SignedInt::build(4);
const auto t_f8 = IR::Float::build(8);
const auto t_char = IR::Char::build();
const auto t_date = IR::Date::build();

size_t getScanIndex(std::string_view name, const std::vector<std::string>& cols) {
   auto elem = std::find(cols.begin(), cols.end(), name);
   auto distance = std::distance(cols.begin(), elem);
   if (distance == cols.size()) {
      throw std::runtime_error("Column " + std::string{name} + " does not exist in scan.");
   }
   return distance;
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

StoredRelationPtr tableSupplier() {
   auto rel = std::make_unique<StoredRelation>();
   rel->attachPODColumn("s_suppkey", t_i4);
   rel->attachStringColumn("s_name");
   rel->attachStringColumn("s_address");
   rel->attachPODColumn("s_nationkey", t_i4);
   rel->attachStringColumn("s_phone");
   rel->attachPODColumn("s_acctbal", t_f8);
   rel->attachStringColumn("s_comment");
   return rel;
}

StoredRelationPtr tableNation() {
   auto rel = std::make_unique<StoredRelation>();
   rel->attachPODColumn("n_nationkey", t_i4);
   rel->attachStringColumn("n_name");
   rel->attachPODColumn("n_regionkey", t_i4);
   rel->attachStringColumn("n_comment");
   return rel;
}

StoredRelationPtr tableRegion() {
   auto rel = std::make_unique<StoredRelation>();
   rel->attachPODColumn("r_regionkey", t_i4);
   rel->attachStringColumn("r_name");
   rel->attachStringColumn("r_comment");
   return rel;
}
}

Schema getTPCHSchema() {
   Schema schema;
   schema.emplace("lineitem", tableLineitem());
   schema.emplace("orders", tableOrders());
   schema.emplace("customer", tableCustomer());
   schema.emplace("part", tablePart());
   schema.emplace("supplier", tableSupplier());
   schema.emplace("nation", tableNation());
   schema.emplace("region", tableRegion());
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

std::unique_ptr<Print> q5(const Schema& schema) {
   // 1.1 Scan from regions.
   auto& region_rel = schema.at("region");
   std::vector<std::string> region_cols{
      "r_regionkey",
      "r_name",
   };
   auto region_scan = TableScan::build(*region_rel, region_cols, "scan_region");
   auto& region_scan_ref = *region_scan;

   // 1.2 Filter on r_name = 'Asia'
   std::vector<ExpressionOp::NodePtr> r_nodes;
   r_nodes.emplace_back(std::make_unique<IURefNode>(region_scan_ref.getOutput()[1]));
   auto r_eq = r_nodes.emplace_back(
                         std::make_unique<ComputeNode>(
                            ComputeNode::Type::StrEquals,
                            IR::StringVal::build("ASIA"),
                            r_nodes[0].get()))
                  .get();

   std::vector<RelAlgOpPtr> children_r_expr;
   children_r_expr.push_back(std::move(region_scan));

   auto r_expr_node = ExpressionOp::build(
      std::move(children_r_expr),
      "region_filter",
      std::vector<Node*>{r_eq},
      std::move(r_nodes));
   auto& r_expr_ref = *r_expr_node;
   assert(r_expr_ref.getOutput().size() == 1);

   std::vector<RelAlgOpPtr> children_r_filter;
   children_r_filter.push_back(std::move(r_expr_node));
   std::vector<const IU*> r_redefined{
      region_scan_ref.getOutput()[getScanIndex("r_regionkey", region_cols)]};
   auto r_filter = Filter::build(std::move(children_r_filter), "r_filter", std::move(r_redefined), *r_expr_ref.getOutput()[0]);
   auto& r_filter_ref = *r_filter;
   assert(r_filter->getOutput().size() == 1);

   // 2. Scan from nations.
   auto& nation_rel = schema.at("nation");
   std::vector<std::string> nation_cols{
      "n_nationkey",
      "n_regionkey",
      "n_name",
   };
   auto nation_scan = TableScan::build(*nation_rel, nation_cols, "scan_region");
   auto& nation_scan_ref = *nation_scan;

   // 3. Join r_regionkey = n_regionkey, returning n_nationkey
   std::vector<RelAlgOpPtr> r_n_join_children;
   r_n_join_children.push_back(std::move(r_filter));
   r_n_join_children.push_back(std::move(nation_scan));
   auto r_n_join = Join::build(
      std::move(r_n_join_children),
      "r_n_join",
      // Keys left (r_regionkey)
      {r_filter_ref.getOutput()[0]},
      // Payload left (none)
      {},
      // Keys right (n_regionkey)
      {nation_scan_ref.getOutput()[1]},
      // Right payload (n_nationkey, n_name)
      {
         nation_scan_ref.getOutput()[0],
         nation_scan_ref.getOutput()[2],
      },
      JoinType::Inner,
      true);
   auto& r_n_join_ref = *r_n_join;
   assert(r_n_join_ref.getOutput().size() == 4);

   // 4. Scan from customer.
   auto& customer_rel = schema.at("customer");
   std::vector<std::string> customer_cols{
      "c_custkey",
      "c_nationkey",
   };
   auto customer_scan = TableScan::build(*customer_rel, customer_cols, "scan_customer");
   auto& customer_scan_ref = *customer_scan;

   // 5. Join customer onto the nationkeys.
   std::vector<RelAlgOpPtr> n_c_join_children;
   n_c_join_children.push_back(std::move(r_n_join));
   n_c_join_children.push_back(std::move(customer_scan));
   auto n_c_join = Join::build(
      std::move(n_c_join_children),
      "n_c_join",
      // Keys left (n_nationkey)
      {r_n_join_ref.getOutput()[2]},
      // Payload left (n_name)
      {r_n_join_ref.getOutput()[3]},
      // Keys right (c_nationkey)
      {customer_scan_ref.getOutput()[1]},
      // Right payload (c_custkey)
      {customer_scan_ref.getOutput()[0]},
      JoinType::Inner,
      true);
   auto& n_c_join_ref = *n_c_join;
   assert(n_c_join_ref.getOutput().size() == 4);

   // 6.1 Scan from orders.
   auto& orders_rel = schema.at("orders");
   std::vector<std::string> orders_cols{
      "o_orderkey",
      "o_custkey",
      "o_orderdate",
   };
   auto orders_scan = TableScan::build(*orders_rel, orders_cols, "scan_orders");
   auto& orders_scan_ref = *orders_scan;

   // 6.2 Filter
   std::vector<ExpressionOp::NodePtr> o_nodes;
   o_nodes.emplace_back(std::make_unique<IURefNode>(orders_scan_ref.getOutput()[2]));
   // 6.2.1 o_orderdate >= '1994-01-01'
   o_nodes.emplace_back(
      std::make_unique<ComputeNode>(
         ComputeNode::Type::LessEqual,
         IR::DateVal::build(helpers::dateStrToInt("1994-01-01")),
         o_nodes[0].get()));
   // 6.2.2 o_orderdate < '1995-01-01'
   o_nodes.emplace_back(
      std::make_unique<ComputeNode>(
         ComputeNode::Type::Greater,
         IR::DateVal::build(helpers::dateStrToInt("1995-01-01")),
         o_nodes[0].get()));
   o_nodes.emplace_back(
      std::make_unique<ComputeNode>(
         ComputeNode::Type::And,
         std::vector<Node*>{o_nodes[1].get(), o_nodes[2].get()}));

   std::vector<RelAlgOpPtr> children_o_expr;
   children_o_expr.push_back(std::move(orders_scan));
   auto o_expr_node = ExpressionOp::build(
      std::move(children_o_expr),
      "orders_filter",
      std::vector<Node*>{o_nodes[3].get()},
      std::move(o_nodes));
   auto& o_expr_ref = *o_expr_node;
   assert(o_expr_ref.getOutput().size() == 1);

   std::vector<RelAlgOpPtr> children_o_filter;
   children_o_filter.push_back(std::move(o_expr_node));
   std::vector<const IU*> o_redefined{
      orders_scan_ref.getOutput()[getScanIndex("o_orderkey", orders_cols)],
      orders_scan_ref.getOutput()[getScanIndex("o_custkey", orders_cols)],
   };
   auto o_filter = Filter::build(std::move(children_o_filter), "o_filter", std::move(o_redefined), *o_expr_ref.getOutput()[0]);
   auto& o_filter_ref = *o_filter;
   assert(o_filter->getOutput().size() == 2);

   // 7. Join customer <-> orders
   std::vector<RelAlgOpPtr> c_o_join_children;
   c_o_join_children.push_back(std::move(n_c_join));
   c_o_join_children.push_back(std::move(o_filter));
   auto c_o_join = Join::build(
      std::move(c_o_join_children),
      "c_o_join",
      // Keys left (c_custkey)
      {n_c_join_ref.getOutput()[3]},
      // Payload left (n_name, c_nationkey)
      {
         n_c_join_ref.getOutput()[1],
         n_c_join_ref.getOutput()[2],
      },
      // Keys right (o_custkey)
      {o_filter_ref.getOutput()[1]},
      // Right payload (o_orderkey)
      {o_filter_ref.getOutput()[0]},
      JoinType::Inner,
      true);
   auto& c_o_join_ref = *c_o_join;
   assert(c_o_join_ref.getOutput().size() == 5);

   // 8. Scan lineitem
   auto& lineitem_rel = schema.at("lineitem");
   std::vector<std::string> lineitem_cols{
      "l_orderkey",
      "l_suppkey",
      "l_extendedprice",
      "l_discount",
   };
   auto lineitem_scan = TableScan::build(*lineitem_rel, lineitem_cols, "scan_lineitem");
   auto& lineitem_scan_ref = *lineitem_scan;

   // 9. Join orders <-> lineitem
   std::vector<RelAlgOpPtr> o_l_join_children;
   o_l_join_children.push_back(std::move(c_o_join));
   o_l_join_children.push_back(std::move(lineitem_scan));
   auto o_l_join = Join::build(
      std::move(o_l_join_children),
      "o_l_join",
      // Keys left (o_orderkey)
      {c_o_join_ref.getOutput()[4]},
      // Payload left (n_name, c_nationkey)
      {
         c_o_join_ref.getOutput()[1],
         c_o_join_ref.getOutput()[2],
      },
      // Keys right (l_orderkey)
      {lineitem_scan_ref.getOutput()[0]},
      // Right payload (l_suppkey, l_extendedprice, l_discount)
      {
         lineitem_scan_ref.getOutput()[1],
         lineitem_scan_ref.getOutput()[2],
         lineitem_scan_ref.getOutput()[3],
      },
      JoinType::Inner,
      true);
   auto& o_l_join_ref = *o_l_join;
   assert(o_l_join_ref.getOutput().size() == 7);

   // 10. Scan from Supplier.
   auto& supplier_rel = schema.at("supplier");
   std::vector<std::string> supplier_cols{
      "s_suppkey",
      "s_nationkey",
   };
   auto supplier_scan = TableScan::build(*supplier_rel, supplier_cols, "scan_supplier");
   auto& supplier_scan_ref = *supplier_scan;

   // 11. Join supplier <-> lineitem
   std::vector<RelAlgOpPtr> s_l_join_children;
   s_l_join_children.push_back(std::move(supplier_scan));
   s_l_join_children.push_back(std::move(o_l_join));
   auto s_l_join = Join::build(
      std::move(s_l_join_children),
      "s_l_join",
      // Keys left (s_suppkey, s_nationkey)
      {
         supplier_scan_ref.getOutput()[0],
         supplier_scan_ref.getOutput()[1],
      },
      // Payload left (none)
      {},
      // Keys right (l_suppkey, c_nationkey)
      {
         o_l_join_ref.getOutput()[4],
         o_l_join_ref.getOutput()[2],
      },
      // Right payload (n_name, l_extendedprice, l_discount)
      {
         o_l_join_ref.getOutput()[1],
         o_l_join_ref.getOutput()[5],
         o_l_join_ref.getOutput()[6],
      },
      JoinType::Inner,
      true);
   auto& s_l_join_ref = *s_l_join;
   assert(s_l_join_ref.getOutput().size() == 7);

   // 12. Aggregate (n_name, sum(l_extendedprice * (1 - l_discount)))
   // 12.1 Expression
   std::vector<ExpressionOp::NodePtr> agg_expr_nodes;
   agg_expr_nodes.emplace_back(std::make_unique<IURefNode>(s_l_join_ref.getOutput()[5]));
   agg_expr_nodes.emplace_back(std::make_unique<IURefNode>(s_l_join_ref.getOutput()[6]));
   agg_expr_nodes.emplace_back(
      std::make_unique<ComputeNode>(
         ComputeNode::Type::Subtract,
         IR::F8::build(1.0),
         agg_expr_nodes[1].get()));
   agg_expr_nodes.emplace_back(
      std::make_unique<ComputeNode>(
         ComputeNode::Type::Multiply,
         std::vector<ExpressionOp::Node*>{agg_expr_nodes[0].get(),
                                          agg_expr_nodes[2].get()}));
   auto agg_expr_root = agg_expr_nodes[3].get();
   std::vector<RelAlgOpPtr> agg_expr_children;
   agg_expr_children.push_back(std::move(s_l_join));
   auto agg_expr = ExpressionOp::build(
      std::move(agg_expr_children),
      "expr_pre_agg",
      {agg_expr_root},
      std::move(agg_expr_nodes));
   auto& agg_expr_ref = *agg_expr;

   // 12.2 GROUP BY
   std::vector<RelAlgOpPtr> agg_children;
   agg_children.push_back(std::move(agg_expr));
   // Group by n_name
   std::vector<const IU*> group_by{s_l_join_ref.getOutput()[4]};
   std::vector<AggregateFunctions::Description> aggregates{
      {*agg_expr_ref.getOutput()[0], AggregateFunctions::Opcode::Sum}};
   auto agg = Aggregation::build(
      std::move(agg_children),
      "agg",
      std::move(group_by),
      std::move(aggregates));

   // Print result.
   std::vector<const IU*> out_ius{
      agg->getOutput()[0],
      agg->getOutput()[1],
   };
   std::vector<std::string> colnames = {
      "n_name",
      "revenue",
   };
   std::vector<RelAlgOpPtr> print_children;
   print_children.push_back(std::move(agg));
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

std::unique_ptr<Print> q19(const Schema& schema) {
   // Build a branch for a part condition.
   auto build_part_branch = [](
                               std::vector<ExpressionOp::NodePtr>& pred_nodes,
                               Node* p_brand_ref,
                               Node* p_size_ref,
                               Node* p_container_ref,
                               std::string brand,
                               std::pair<int32_t, int32_t>
                                  size_between,
                               std::vector<std::string>
                                  container_list) -> Node* {
      auto p_brand_pred = pred_nodes.emplace_back(
                                       std::make_unique<ComputeNode>(
                                          ComputeNode::Type::StrEquals,
                                          IR::StringVal::build(brand),
                                          p_brand_ref))
                             .get();
      auto p_size_pred_1 = pred_nodes.emplace_back(
                                        std::make_unique<ComputeNode>(
                                           ComputeNode::Type::GreaterEqual,
                                           IR::SI<4>::build(size_between.second),
                                           p_size_ref))
                              .get();
      auto p_size_pred_2 = pred_nodes.emplace_back(
                                        std::make_unique<ComputeNode>(
                                           ComputeNode::Type::LessEqual,
                                           IR::SI<4>::build(size_between.first),
                                           p_size_ref))
                              .get();

      auto p_container_pred = pred_nodes.emplace_back(
                                           std::make_unique<ComputeNode>(
                                              ComputeNode::Type::InList,
                                              IR::StringList::build(container_list),
                                              p_container_ref))
                                 .get();

      return Andify(pred_nodes,
                    {p_size_pred_1, p_size_pred_2, p_brand_pred, p_container_pred});
   };

   // Build a branch for a lineitem condition.
   auto build_lineitem_branch = [](
                                   std::vector<ExpressionOp::NodePtr>& pred_nodes,
                                   Node* l_quantity_ref,
                                   std::pair<int32_t, int32_t>
                                      l_quantity_between) -> Node* {
      auto l_quantity_pred_1 = pred_nodes.emplace_back(
                                            std::make_unique<ComputeNode>(
                                               ComputeNode::Type::GreaterEqual,
                                               IR::F8::build(l_quantity_between.second + 0.001),
                                               l_quantity_ref))
                                  .get();
      auto l_quantity_pred_2 = pred_nodes.emplace_back(
                                            std::make_unique<ComputeNode>(
                                               ComputeNode::Type::LessEqual,
                                               IR::F8::build(l_quantity_between.first - 0.001),
                                               l_quantity_ref))
                                  .get();

      return Andify(pred_nodes,
                    {l_quantity_pred_1, l_quantity_pred_2});
   };

   // 1. Scan part.
   auto& rel_p = schema.at("part");
   std::vector<std::string> cols_p{
      "p_partkey",
      "p_brand",
      "p_container",
      "p_size",
   };
   auto scan_p = TableScan::build(*rel_p, cols_p, "scan_part");
   auto& scan_p_ref = *scan_p;

   // 2. Pushed down filter on part.
   std::vector<ExpressionOp::NodePtr> pred_nodes_p;
   auto p_partkey_ref = pred_nodes_p.emplace_back(std::make_unique<IURefNode>(
                                                     scan_p_ref.getOutput()[getScanIndex("p_partkey", cols_p)]))
                           .get();
   auto p_brand_ref = pred_nodes_p.emplace_back(std::make_unique<IURefNode>(
                                                   scan_p_ref.getOutput()[getScanIndex("p_brand", cols_p)]))
                         .get();
   auto p_container_ref = pred_nodes_p.emplace_back(std::make_unique<IURefNode>(
                                                       scan_p_ref.getOutput()[getScanIndex("p_container", cols_p)]))
                             .get();
   auto p_size_ref = pred_nodes_p.emplace_back(std::make_unique<IURefNode>(
                                                  scan_p_ref.getOutput()[getScanIndex("p_size", cols_p)]))
                        .get();

   // OR branch 1
   Node* scan_p_branch_1 = build_part_branch(
      pred_nodes_p,
      p_brand_ref,
      p_size_ref,
      p_container_ref,
      "Brand#12",
      {1, 5},
      {"SM CASE", "SM BOX", "SM PACK", "SM PKG"});
   // OR branch 1
   Node* scan_p_branch_2 = build_part_branch(
      pred_nodes_p,
      p_brand_ref,
      p_size_ref,
      p_container_ref,
      "Brand#23",
      {1, 10},
      {"MED BAG", "MED BOX", "MED PKG", "MED PACK"});
   // OR branch 1
   Node* scan_p_branch_3 = build_part_branch(
      pred_nodes_p,
      p_brand_ref,
      p_size_ref,
      p_container_ref,
      "Brand#34",
      {1, 15},
      {"LG CASE", "LG BOX", "LG PACK", "LG PKG"});

   // OR it all together.
   auto scan_p_filter = Orify(
      pred_nodes_p, {scan_p_branch_1, scan_p_branch_2, scan_p_branch_3});

   std::vector<RelAlgOpPtr> expr_p_children;
   expr_p_children.push_back(std::move(scan_p));
   auto expr_p_node = ExpressionOp::build(
      std::move(expr_p_children),
      "part_filter",
      std::vector<Node*>{scan_p_filter},
      std::move(pred_nodes_p));
   auto& expr_p_ref = *expr_p_node;
   assert(expr_p_ref.getOutput().size() == 1);

   std::vector<RelAlgOpPtr> filter_p_children;
   filter_p_children.push_back(std::move(expr_p_node));
   std::vector<const IU*> filter_p_redefined{
      scan_p_ref.getOutput()[0],
      scan_p_ref.getOutput()[1],
      scan_p_ref.getOutput()[2],
      scan_p_ref.getOutput()[3],
   };
   auto filter_p = Filter::build(
      std::move(filter_p_children),
      "filter_p",
      std::move(filter_p_redefined),
      *expr_p_ref.getOutput()[0]);
   auto& filter_p_ref = *filter_p;
   assert(filter_p->getOutput().size() == 4);

   // 3. Scan lineitem.
   auto& rel_l = schema.at("lineitem");
   std::vector<std::string> cols_l{
      "l_partkey",
      "l_shipmode",
      "l_quantity",
      "l_shipinstruct",
      "l_discount",
      "l_extendedprice",
   };

   // 4. Pushed down lineitem filter.
   auto scan_l = TableScan::build(*rel_l, cols_l, "scan_lineitem");
   auto& scan_l_ref = *scan_l;

   std::vector<ExpressionOp::NodePtr> pred_nodes_l;
   auto l_partkey_ref = pred_nodes_l.emplace_back(std::make_unique<IURefNode>(
                                                     scan_l_ref.getOutput()[getScanIndex("l_partkey", cols_l)]))
                           .get();
   auto l_shipmode_ref = pred_nodes_l.emplace_back(std::make_unique<IURefNode>(
                                                      scan_l_ref.getOutput()[getScanIndex("l_shipmode", cols_l)]))
                            .get();
   auto l_quantity_ref = pred_nodes_l.emplace_back(std::make_unique<IURefNode>(
                                                      scan_l_ref.getOutput()[getScanIndex("l_quantity", cols_l)]))
                            .get();
   auto l_shipinstruct_ref = pred_nodes_l.emplace_back(std::make_unique<IURefNode>(
                                                          scan_l_ref.getOutput()[getScanIndex("l_shipinstruct", cols_l)]))
                                .get();

   auto l_shipinstruct_pred = pred_nodes_l.emplace_back(
                                             std::make_unique<ComputeNode>(
                                                ComputeNode::Type::StrEquals,
                                                IR::StringVal::build("DELIVER IN PERSON"),
                                                l_shipinstruct_ref))
                                 .get();
   auto l_shipmode_pred = pred_nodes_l.emplace_back(
                                         std::make_unique<ComputeNode>(
                                            ComputeNode::Type::InList,
                                            IR::StringList::build({"AIR", "AIR REG"}),
                                            l_shipmode_ref))
                             .get();

   // OR branch 1
   Node* scan_l_branch_1 = build_lineitem_branch(
      pred_nodes_l,
      l_quantity_ref,
      {1, 11});
   // OR branch 1

   // OR branch 2
   Node* scan_l_branch_2 = build_lineitem_branch(
      pred_nodes_l,
      l_quantity_ref,
      {10, 20});

   Node* scan_l_branch_3 = build_lineitem_branch(
      pred_nodes_l,
      l_quantity_ref,
      {20, 30});

   auto l_common_or = Orify(pred_nodes_l, {scan_l_branch_1, scan_l_branch_2, scan_l_branch_3});

   auto l_common_and = Andify(pred_nodes_l, {l_shipinstruct_pred, l_shipmode_pred, l_common_or});

   std::vector<RelAlgOpPtr> expr_l_children;
   expr_l_children.push_back(std::move(scan_l));
   auto expr_l_node = ExpressionOp::build(
      std::move(expr_l_children),
      "lineitem_filter",
      std::vector<Node*>{l_common_and},
      std::move(pred_nodes_l));
   auto& expr_l_ref = *expr_l_node;
   assert(expr_l_ref.getOutput().size() == 1);

   std::vector<RelAlgOpPtr> filter_l_children;
   filter_l_children.push_back(std::move(expr_l_node));
   std::vector<const IU*> filter_l_redefined{
      scan_l_ref.getOutput()[0],
      scan_l_ref.getOutput()[2],
      // l_shipinstruct and l_shipmode are no longer needed
      scan_l_ref.getOutput()[4],
      scan_l_ref.getOutput()[5],
   };
   auto filter_l = Filter::build(
      std::move(filter_l_children),
      "filter_l",
      std::move(filter_l_redefined),
      *expr_l_ref.getOutput()[0]);
   auto& filter_l_ref = *filter_l;
   assert(filter_l->getOutput().size() == 4);

   // 5. Join the two
   std::vector<RelAlgOpPtr> p_l_join_children;
   p_l_join_children.push_back(std::move(filter_p));
   p_l_join_children.push_back(std::move(filter_l));
   auto p_l_join = Join::build(
      std::move(p_l_join_children),
      "p_l_join",
      // Keys left (p_partkey)
      {filter_p_ref.getOutput()[0]},
      // Payload left (p_brand, p_container, p_size)
      {
         filter_p_ref.getOutput()[1],
         filter_p_ref.getOutput()[2],
         filter_p_ref.getOutput()[3],
      },
      // Keys right (l_partkey)
      {filter_l_ref.getOutput()[0]},
      // Payload right (l_quantity, l_discount, l_extendedprice)
      {
         filter_l_ref.getOutput()[1],
         filter_l_ref.getOutput()[2],
         filter_l_ref.getOutput()[3],
      },
      JoinType::Inner,
      true);
   auto& p_l_join_ref = *p_l_join;

   // 6. Filter again, we need to make sure the right tuples survived.
   std::vector<ExpressionOp::NodePtr> pred_nodes_p_l;
   auto p_l_brand_ref = pred_nodes_p_l.emplace_back(std::make_unique<IURefNode>(
                                                       p_l_join_ref.getOutput()[1]))
                           .get();
   auto p_l_container_ref = pred_nodes_p_l.emplace_back(std::make_unique<IURefNode>(
                                                           p_l_join_ref.getOutput()[2]))
                               .get();
   auto p_l_size_ref = pred_nodes_p_l.emplace_back(std::make_unique<IURefNode>(
                                                      p_l_join_ref.getOutput()[3]))
                          .get();
   auto p_l_quantity_ref = pred_nodes_p_l.emplace_back(std::make_unique<IURefNode>(
                                                          p_l_join_ref.getOutput()[5]))
                              .get();

   // Or branch 1
   Node* p_l_branch_1_l = build_lineitem_branch(
      pred_nodes_p_l,
      p_l_quantity_ref,
      {1, 11});
   Node* p_l_branch_1_p = build_part_branch(
      pred_nodes_p_l,
      p_l_brand_ref,
      p_l_size_ref,
      p_l_container_ref,
      "Brand#12",
      {1, 5},
      {"SM CASE", "SM BOX", "SM PACK", "SM PKG"});
   auto p_l_branch_1 = Andify(pred_nodes_p_l, {p_l_branch_1_l, p_l_branch_1_p});

   // Or branch 2
   Node* p_l_branch_2_l = build_lineitem_branch(
      pred_nodes_p_l,
      p_l_quantity_ref,
      {10, 20});
   Node* p_l_branch_2_p = build_part_branch(
      pred_nodes_p_l,
      p_l_brand_ref,
      p_l_size_ref,
      p_l_container_ref,
      "Brand#23",
      {1, 10},
      {"MED BAG", "MED BOX", "MED PKG", "MED PACK"});
   auto p_l_branch_2 = Andify(pred_nodes_p_l, {p_l_branch_2_l, p_l_branch_2_p});

   // Or branch 3
   Node* p_l_branch_3_l = build_lineitem_branch(
      pred_nodes_p_l,
      p_l_quantity_ref,
      {20, 30});
   Node* p_l_branch_3_p = build_part_branch(
      pred_nodes_p_l,
      p_l_brand_ref,
      p_l_size_ref,
      p_l_container_ref,
      "Brand#34",
      {1, 15},
      {"LG CASE", "LG BOX", "LG PACK", "LG PKG"});
   auto p_l_branch_3 = Andify(pred_nodes_p_l, {p_l_branch_3_l, p_l_branch_3_p});

   auto p_l_common_or = Orify(pred_nodes_p_l, {p_l_branch_1, p_l_branch_2, p_l_branch_3});

   std::vector<RelAlgOpPtr> expr_p_l_children;
   expr_p_l_children.push_back(std::move(p_l_join));
   auto expr_p_l_node = ExpressionOp::build(
      std::move(expr_p_l_children),
      "final_filter",
      std::vector<Node*>{p_l_common_or},
      std::move(pred_nodes_p_l));
   auto& expr_p_l_ref = *expr_p_l_node;
   assert(expr_p_l_ref.getOutput().size() == 1);

   std::vector<RelAlgOpPtr> filter_p_l_children;
   filter_p_l_children.push_back(std::move(expr_p_l_node));
   std::vector<const IU*> filter_p_l_redefined{
      // l_discount, l_extendedprice
      p_l_join_ref.getOutput()[6],
      p_l_join_ref.getOutput()[7],
   };
   auto filter_p_l = Filter::build(
      std::move(filter_p_l_children),
      "filter_p_l",
      std::move(filter_p_l_redefined),
      *expr_p_l_ref.getOutput()[0]);
   auto& filter_p_l_ref = *filter_p_l;
   assert(filter_p_l->getOutput().size() == 2);

   // 7. Aggregate the result.
   // 7.1 Compute (l_extendedprice * (1 - l_discount))
   std::vector<ExpressionOp::NodePtr> agg_nodes;
   agg_nodes.emplace_back(std::make_unique<IURefNode>(filter_p_l_ref.getOutput()[0]));
   agg_nodes.emplace_back(std::make_unique<IURefNode>(filter_p_l_ref.getOutput()[1]));
   agg_nodes.emplace_back(std::make_unique<ComputeNode>(ComputeNode::Type::Subtract, IR::F8::build(1.0), agg_nodes[0].get()));
   agg_nodes.emplace_back(std::make_unique<ComputeNode>(ComputeNode::Type::Multiply, std::vector<Node*>{agg_nodes[1].get(), agg_nodes[2].get()}));
   auto agg_nodes_root = agg_nodes[3].get();
   std::vector<RelAlgOpPtr> agg_e_children;
   agg_e_children.push_back(std::move(filter_p_l));
   auto agg_e = ExpressionOp::build(
      std::move(agg_e_children),
      "agg_expr",
      {agg_nodes_root},
      std::move(agg_nodes));
   auto& agg_e_ref = *agg_e;

   // 7.2 Perform the aggregation
   std::vector<RelAlgOpPtr> agg_children;
   agg_children.push_back(std::move(agg_e));
   // Empty group-by key 
   std::vector<const IU*> group_by{};
   std::vector<AggregateFunctions::Description> aggregates{
      {*agg_e_ref.getOutput()[0], AggregateFunctions::Opcode::Sum}};
   auto agg = Aggregation::build(
      std::move(agg_children),
      "agg",
      std::move(group_by),
      std::move(aggregates));

   // Print result.
   std::vector<const IU*> out_ius{
      agg->getOutput()[0],
   };
   std::vector<std::string> colnames = {
      "revenue",
   };
   std::vector<RelAlgOpPtr> print_children;
   print_children.push_back(std::move(agg));
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
