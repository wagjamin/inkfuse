// ---------------------------------------------------------------------------
#include "imlab/semana.h"
#include "imlab/algebra/table_scan.h"
#include "imlab/algebra/expression.h"
#include "imlab/algebra/inner_join.h"
#include "imlab/algebra/selection.h"
#include "imlab/algebra/print.h"
// ---------------------------------------------------------------------------
namespace imlab {
// ---------------------------------------------------------------------------
namespace {
// ---------------------------------------------------------------------------
const StoredRelation& getRelation(const std::string& name, const Database& db) {
    if (name == "warehouse") {
        return db.warehouse;
    } else if (name == "district") {
        return db.district;
    } else if (name == "customer") {
        return db.customer;
    } else if (name == "history") {
        return db.history;
    } else if (name == "neworder") {
        return db.neworder;
    } else if (name == "order") {
        return db.order;
    } else if (name == "orderline") {
        return db.orderline;
    } else if (name == "item") {
        return db.item;
    } else if (name == "stock") {
        return db.stock;
    } else {
        throw std::runtime_error("requested table " + name + " does not exist.");
    }
}
// ---------------------------------------------------------------------------
/// Get the produced IU for a column as well as the offset into the producer list.
std::pair<size_t, const IU*> getIUProducer(const std::string& colname, const std::vector<OperatorPtr>& operators) {
    for (auto op = operators.begin(); op < operators.end(); ++op) {
        auto idx = std::distance(operators.begin(), op);
        auto ius = (*op)->CollectIUs();
        for (const auto& iu: ius) {
            if (iu->column == colname) {
                // Using assumption that attribute names are unique.
                return {idx, iu};
            }
        }
    }
    throw std::runtime_error("requested column " + colname + " does not exist.");
}
// ---------------------------------------------------------------------------
}
// ---------------------------------------------------------------------------
std::unique_ptr<Operator> Semana::analyze(CodegenHelper& codegen, const queryc::QueryAST &ast, const Database &db)
{
    // The current operators.
    std::vector<OperatorPtr> operators;
    // Set up table scans.
    for (const auto& table: ast.from_list) {
        OperatorPtr scan = std::make_unique<TableScan>(codegen, getRelation(table, db), table.c_str());
        operators.push_back(std::move(scan));
    }
    using PredDesc = std::tuple<std::string, std::string, queryc::PredType>;
    // Analyze filters.
    std::vector<PredDesc> pushed_down;
    std::vector<PredDesc> join_cond;
    for (const auto& elem: ast.where_list) {
        if (std::get<2>(elem) == queryc::PredType::ColumnRef) {
            // Join condition.
            join_cond.push_back(elem);
        } else {
            // Pushed down condition onto a base relation.
            pushed_down.push_back(elem);
        }
    }
    // Setup base scans.
    for (const auto& p_filter: pushed_down) {
        // We first have to find the correct table scan.
        auto [idx, iu] = getIUProducer(std::get<0>(p_filter), operators);
        // Set up expression.
        ExpressionPtr ref = std::make_unique<IURefExpression>(*iu);
        std::string constant_string = std::get<1>(p_filter);
        if (std::get<2>(p_filter) == queryc::PredType::StringConstant) {
            // Quote string constant.
            constant_string = "\"" + constant_string + "\"";
        }
        ExpressionPtr constant = std::make_unique<ConstantExpression>(iu->type, iu->type.Desc() + "{" + constant_string + "}");
        ExpressionPtr equals = std::make_unique<EqualsExpression>(std::move(ref), std::move(constant));
        OperatorPtr filtered = std::make_unique<Selection>(codegen, std::move(operators[idx]), std::move(equals));
        operators[idx] = std::move(filtered);
    }
    // Setup the join tree above the filtered base relations.
    // We do not worry about join ordering here, but just correctness.
    while (!join_cond.empty()) {
        std::vector<std::pair<const IU*, const IU*>> pred;
        // Copy by value is important here as the vector gets changed below.
        auto p_join = join_cond.back();
        join_cond.pop_back();
        // This is an extremely inefficient way to do this, as it has squared runtime.
        // But considering how many other things are slow (e.g. compilation) and that we don't consider massive join trees,
        // this isn't going to kill us.

        // Get the operators which are referenced by this condition.
        auto col_left = std::get<0>(p_join);
        auto col_right = std::get<1>(p_join);
        auto [idx_l, iu_l] = getIUProducer(col_left, operators);
        auto [idx_r, iu_r] = getIUProducer(col_right, operators);
        if (idx_l == idx_r) {
            throw std::runtime_error("Column equality predicates coming from same relation. Something went tragically wrong.");
        }
        pred.emplace_back(iu_l, iu_r);

        // Now to the ugly part, we have to find all other predicates referring to a join for the same subtrees.
        for (auto other = join_cond.begin(); other < join_cond.end();) {
            auto curr_join = *other;
            auto col_left_curr = std::get<0>(curr_join);
            auto col_right_curr = std::get<1>(curr_join);
            auto [idx_l_o, iu_l_o] = getIUProducer(col_left_curr, operators);
            auto [idx_r_o, iu_r_o] = getIUProducer(col_right_curr, operators);
            if (idx_l == idx_l_o && idx_r == idx_r_o) {
                pred.emplace_back(iu_l_o, iu_r_o);
                other = join_cond.erase(other);
                continue;
            }
            if (idx_r == idx_l_o && idx_l == idx_r_o) {
                pred.emplace_back(iu_r_o, iu_l_o);
                other = join_cond.erase(other);
                continue;
            }
            other++;
        }

        // We are ready to build the join.
        OperatorPtr join = std::make_unique<InnerJoin>(codegen, std::move(operators[idx_l]), std::move(operators[idx_r]), std::move(pred));
        // Update the operator mapping with the new join.
        operators[idx_l] = std::move(join);
        operators.erase(operators.begin() + idx_r);
    }

    if (operators.size() != 1) {
        throw std::runtime_error("Query would require cross product, this is not supported.");
    }

    // Setup the final print operator.
    std::set<const IU*> print_set;
    for (auto& print_col: ast.select_list) {
        auto [idx, iu] = getIUProducer(print_col, operators);
        print_set.insert(iu);
    }
    OperatorPtr print_op = std::make_unique<Print>(codegen, std::move(operators[0]), std::move(print_set));

    return print_op;
}
// ---------------------------------------------------------------------------
} // namespace imlab
// ---------------------------------------------------------------------------