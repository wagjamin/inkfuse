#include <iostream>
#include <fstream>

#include "gflags/gflags.h"
#include "imlab/algebra/inner_join.h"
#include "imlab/algebra/iu.h"
#include "imlab/algebra/operator.h"
#include "imlab/algebra/codegen_helper.h"
#include "imlab/algebra/print.h"
#include "imlab/algebra/selection.h"
#include "imlab/algebra/table_scan.h"
#include "imlab/database.h"

DEFINE_string(out_cc, "", "Header path");
DEFINE_string(out_h, "", "Implementation path");

static bool ValidateWritable(const char *flagname, const std::string &value) {
    std::ofstream out(value);
    return out.good();
}
static bool ValidateReadable(const char *flagname, const std::string &value) {
    std::ifstream in(value);
    return in.good();
}

DEFINE_validator(out_cc, &ValidateWritable);
DEFINE_validator(out_h, &ValidateWritable);

int main(int argc, char *argv[]) {
    gflags::SetUsageMessage("queryc --out_cc <H> --out_h <CC>");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    std::ofstream out_h(FLAGS_out_h, std::ofstream::trunc);
    std::ofstream out_cc(FLAGS_out_cc, std::ofstream::trunc);

    // Write simple header.
    out_h << R"(
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

)";

    out_cc << R"(

#include "gen/query.h"

namespace imlab

)";

    // Prepare codegen helper.
    imlab::CodegenHelper helper(&out_cc);

    auto namespace_scope = helper.CreateScopedBlock();

    out_cc << "\nvoid GeneratedQuery::executeOLAPQuery(Database& db) \n";

    auto function_scope = helper.CreateScopedBlock();

    // Create raw database.
    imlab::Database db;

    // Build algebra tree.
    // Setup the scans
    auto scan_customer = std::make_unique<imlab::TableScan>(helper, db.customer, "customer");
    auto scan_order = std::make_unique<imlab::TableScan>(helper, db.order, "order");
    auto scan_orderline = std::make_unique<imlab::TableScan>(helper, db.orderline, "orderline");

    // Get produced IUs.
    auto customer_ius = scan_customer->CollectIUs();
    auto order_ius = scan_order->CollectIUs();
    auto orderline_ius = scan_orderline->CollectIUs();

    // Setup pushed down filter on customer.
    std::unique_ptr<imlab::Expression> const322 = std::make_unique<imlab::ConstantExpression>(imlab::schemac::Type::Integer(), "Integer{322}");
    std::unique_ptr<imlab::Expression> const1_1 = std::make_unique<imlab::ConstantExpression>(imlab::schemac::Type::Integer(), "Integer{1}");
    std::unique_ptr<imlab::Expression> const1_2 = std::make_unique<imlab::ConstantExpression>(imlab::schemac::Type::Integer(), "Integer{1}");

    std::unique_ptr<imlab::Expression> iuref_c_id = std::make_unique<imlab::IURefExpression>(*customer_ius[0]);
    std::unique_ptr<imlab::Expression> iuref_c_d_id = std::make_unique<imlab::IURefExpression>(*customer_ius[1]);
    std::unique_ptr<imlab::Expression> iuref_c_w_id = std::make_unique<imlab::IURefExpression>(*customer_ius[2]);

    std::unique_ptr<imlab::Expression> eq_c_id = std::make_unique<imlab::EqualsExpression>(std::move(iuref_c_id), std::move(const322));
    std::unique_ptr<imlab::Expression> eq_c_d_id = std::make_unique<imlab::EqualsExpression>(std::move(iuref_c_d_id), std::move(const1_1));
    std::unique_ptr<imlab::Expression> eq_c_w_id = std::make_unique<imlab::EqualsExpression>(std::move(iuref_c_w_id), std::move(const1_2));

    std::unique_ptr<imlab::Expression> and_1_expr = std::make_unique<imlab::AndExpression>(std::move(eq_c_id), std::move(eq_c_d_id));
    std::unique_ptr<imlab::Expression> and_2_expr = std::make_unique<imlab::AndExpression>(std::move(and_1_expr), std::move(eq_c_w_id));

    auto filter_customer = std::make_unique<imlab::Selection>(helper, std::move(scan_customer), std::move(and_2_expr));

    // Setup join of cust -> order
    std::vector<std::pair<const imlab::IU*, const imlab::IU*>> join_pred_cust_order;
    join_pred_cust_order.emplace_back(customer_ius[2], order_ius[2]);
    join_pred_cust_order.emplace_back(customer_ius[1], order_ius[1]);
    join_pred_cust_order.emplace_back(customer_ius[0], order_ius[0]);
    auto join_cust_oder = std::make_unique<imlab::InnerJoin>(helper, std::move(filter_customer), std::move(scan_order), std::move(join_pred_cust_order));

    // Setup join of (cust -> order) -> orderline
    std::vector<std::pair<const imlab::IU*, const imlab::IU*>> join_pred_order_orderline;
    join_pred_order_orderline.emplace_back(order_ius[2], orderline_ius[2]);
    join_pred_order_orderline.emplace_back(order_ius[1], orderline_ius[1]);
    join_pred_order_orderline.emplace_back(order_ius[0], orderline_ius[0]);
    auto join_order_oderline = std::make_unique<imlab::InnerJoin>(helper, std::move(join_cust_oder), std::move(scan_orderline), std::move(join_pred_order_orderline));

    // Setup final printer.
    std::set<const imlab::IU*> print_ius{customer_ius[3], customer_ius[5], order_ius[7], orderline_ius[8]};
    auto printer = std::make_unique<imlab::Print>(helper, std::move(join_order_oderline), print_ius);
    printer->Prepare(std::set<const imlab::IU*>{}, nullptr);
    printer->Produce();
}

