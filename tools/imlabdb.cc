// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------

#include "imlab/database.h"
#include "imlab/infra/types.h"
#include "imlab/schema.h"
#include "imlab/semana.h"
#include "query_parse_context.h"
#include "code_generator.h"
#include "gflags/gflags.h"
#include <atomic>
#include <iostream>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <thread>
#include <chrono> // NOLINT
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>

namespace {

// A function to return a seeded random number generator.
inline std::mt19937& generator() {
   // the generator will only be seeded once (per thread) since it's static
   static thread_local std::mt19937 gen(std::random_device{}());
   return gen;
}

// A function to generate integers in the range [min, max]
template <typename T, std::enable_if_t<std::is_integral_v<T>>* = nullptr>
T rand(T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max()) {
   std::uniform_int_distribution<T> dist(min, max);
   return dist(generator());
}

// A function to generate floats in the range [min, max)
template <typename T, std::enable_if_t<std::is_floating_point_v<T>>* = nullptr>
T rand(T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max()) {
   std::uniform_real_distribution<T> dist(min, max);
   return dist(generator());
}

const int32_t kWarehouses = 5;

// Uniform random number
int32_t URand(int32_t min, int32_t max) {
   return rand<int32_t>(min, max);
}

// Uniform random number
int32_t URandExcept(int32_t min, int32_t max, int32_t v) {
   if (max <= min) {
      return min;
   }
   int32_t r = rand<int32_t>(min, max - 1);
   return (r >= v) ? (r + 1) : r;
}

// Non-uniform random number
int32_t NURand(int32_t A, int32_t x, int32_t y) {
   return (((URand(0, A) | URand(x, y)) + 42) % (y - x + 1)) + x;
}

// Place a random order
void RandomNewOrder(imlab::Database& db) {
   Timestamp now(0);
   auto w_id = Integer(URand(1, kWarehouses));
   auto d_id = Integer(URand(1, 10));
   auto c_id = Integer(NURand(1023, 1, 3000));
   auto ol_cnt = Integer(URand(5, 15));

   std::array<Integer, 15> supware;
   std::array<Integer, 15> itemid;
   std::array<Integer, 15> qty;
   for (auto i = Integer(0); i < ol_cnt; i += Integer(1)) {
      supware[i.value] = (URand(1, 100) > 1) ? w_id : Integer(URandExcept(1, kWarehouses, w_id.value));
      itemid[i.value] = Integer(NURand(8191, 1, 100000));
      qty[i.value] = Integer(URand(1, 10));
   }

   db.NewOrder(w_id, d_id, c_id, ol_cnt, supware, itemid, qty, now);
}

// Place a random delivery
void RandomDelivery(imlab::Database& db) {
   Timestamp now(0);
   db.Delivery(Integer(URand(1, kWarehouses)), Integer(URand(1, 10)), now);
}

std::atomic<bool> childRunning;

static void SIGCHLD_handler(int /*sig*/) {
    int status;
    wait(&status);
    // now the child with process id childPid is dead
    childRunning = false;
}

// Async OLAP runner
int async_olap(size_t count, imlab::Database& db) {
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags=0;
    sa.sa_handler=SIGCHLD_handler;
    sigaction(SIGCHLD, &sa,NULL);

    for (size_t i=0; i<count; i++) {
        childRunning=true;
        pid_t pid=fork();
        if (pid) { // parent
            while (childRunning); // busy wait for child
        } else { // forked child
            childRunning = false;
            db.AnalyticalQuerySTL();
            exit(0); // Child finished.
        }
    }

    return 0;
}

} // namespace

DEFINE_string(include_dir, "", "Path to imlabdb include directories");

static bool ValidateFlag(const char *flagname, const std::string &value) {
    return !value.empty();
}

DEFINE_validator(include_dir, &ValidateFlag);

int main(int argc, char* argv[]) {
    gflags::SetUsageMessage("imlabdb --include_dir <path to imlab include dir>");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    // Create database.
    imlab::Database db;

    // Load tables.
    std::ifstream stream_cust{"tpcc_customer.tbl"};
    db.LoadCustomer(stream_cust);
    std::ifstream stream_district{"tpcc_district.tbl"};
    db.LoadDistrict(stream_district);
    std::ifstream stream_history{"tpcc_history.tbl"};
    db.LoadHistory(stream_history);
    std::ifstream stream_item{"tpcc_item.tbl"};
    db.LoadItem(stream_item);
    std::ifstream stream_neworder{"tpcc_neworder.tbl"};
    db.LoadNewOrder(stream_neworder);
    std::ifstream stream_order{"tpcc_order.tbl"};
    db.LoadOrder(stream_order);
    std::ifstream stream_orderline{"tpcc_orderline.tbl"};
    db.LoadOrderLine(stream_orderline);
    std::ifstream stream_stock{"tpcc_stock.tbl"};
    db.LoadStock(stream_stock);
    std::ifstream stream_warehouse{"tpcc_warehouse.tbl"};
    db.LoadWarehouse(stream_warehouse);

    std::cout << "Tuples in table district: " << db.Size<imlab::tpcc::kDistrict>()<< "\n";
    std::cout << "Tuples in table order: " << db.Size<imlab::tpcc::kOrder>()<< "\n";
    std::cout << "Tuples in table neworder: " << db.Size<imlab::tpcc::kNewOrder>()<< "\n";
    std::cout << "Tuples in table stock: " << db.Size<imlab::tpcc::kStock>()<< "\n";
    std::cout << "Tuples in table orderline: " << db.Size<imlab::tpcc::kOrderLine>()<< "\n";

    // Commented out code from earlier assignments for completeness.
    /*
    std::cout << "-------------- Running Raw Join Performance -----------------\n";
    {
        auto start = std::chrono::steady_clock::now();
        std::cout << "STL Join OLAP Result: " << db.AnalyticalQuerySTL() << std::endl;
        auto stop = std::chrono::steady_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        std::cout << "Total join STL duration: " << static_cast<double>(dur) / 1000.0 << std::endl;
    }
    {
        auto start = std::chrono::steady_clock::now();
        std::cout << "LHT Join OLAP Result: " << db.AnalyticalQueryLHT() << std::endl;
        auto stop = std::chrono::steady_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        std::cout << "Total join duration: " << static_cast<double>(dur) / 1000.0 << std::endl;
    }

    {
        auto start = std::chrono::steady_clock::now();
        std::cout << "Running Jitted Query\n";
        imlab::GeneratedQuery::executeOLAPQuery(db);
        auto stop = std::chrono::steady_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        std::cout << "Total join duration: " << static_cast<double>(dur) / 1000.0 << std::endl;
    }

    std::cout << "-------------- Done-----------------\n";


    std::cout << "-------------- Running Load Experiments -----------------\n";

    auto start = std::chrono::steady_clock::now();

    // TASK 1 - New Order Only
    // Run one million new order transactions.
    for (size_t i = 0; i < 1000 * 1000; ++i) {
        RandomNewOrder(db);
    }

    // TASK 2 - Mix New Order and Delivery
    // Run one million delivery transactions.
    for (size_t i = 0; i < 1000 * 1000; ++i) {
        if (rand<uint32_t>(1, 10) == 1) {
            RandomNewOrder(db);
        } else {
            RandomDelivery(db);
        }
    }

    // TASK 3 - Mixed OLAP query and New Order / Delivery Mix
    auto thread = std::thread([&](){async_olap(10, db);});
    for (size_t i = 0; i < 1000 * 1000; ++i) {
        if (rand<uint32_t>(1, 10) == 1) {
            RandomNewOrder(db);
        } else {
            RandomDelivery(db);
        }
    }
    thread.join();

    auto stop = std::chrono::steady_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();

    std::cout << "-------------- Done-----------------\n";

    std::cout << "1 Million transactions took " << dur << " milliseconds\n";
    std::cout << "TPS: " << (1000.0 * 1000.0 * 1000.0 + 10) / static_cast<double>(dur) << "\n";

    std::cout << "Tuples in updated table district: " << db.Size<imlab::tpcc::kDistrict>()<< "\n";
    std::cout << "Tuples in updated table order: " << db.Size<imlab::tpcc::kOrder>()<< "\n";
    std::cout << "Tuples in updated table neworder: " << db.Size<imlab::tpcc::kNewOrder>()<< "\n";
    std::cout << "Tuples in updated table stock: " << db.Size<imlab::tpcc::kStock>()<< "\n";
    std::cout << "Tuples in updated table orderline: " << db.Size<imlab::tpcc::kOrderLine>()<< "\n";
    */

    imlab::queryc::CodeGenerator gen(FLAGS_include_dir);

    // Read lines from stdin containing queries.
    while (true) {
        std::cout << "Enter query to execute - or 'exit;' to quit\n";
        std::string line;
        std::getline(std::cin, line);
        if (line == "exit;") {
            break;
        }
        std::istringstream instream(line);
        // Setup codegen helper.
        imlab::CodegenHelper codegen;
        // Parse.
        imlab::queryc::QueryAST ast;
        try {
            imlab::queryc::QueryParseContext context(false, false);
            ast = context.Parse(instream);
        } catch (const std::exception& e) {
            std::cout << "Parsing failed: " << e.what() << "\n";
            continue;
        }

        imlab::OperatorPtr op;
        try {
            // And analyze.
            op = imlab::Semana::analyze(codegen, ast, db);
        } catch (const std::exception& e) {
            std::cout << "Analysis failed: " << e.what() << "\n";
            continue;
        }

        imlab::queryc::CodeGenerator::Function fn;

        try {
            // Next up we can generate the code.
            fn = gen.compile(std::move(op));
        } catch (const std::exception& e) {
            std::cout << "Codegen failed: " << e.what() << "\n";
            continue;
        }

        try {
            fn(reinterpret_cast<void*>(&db));
        } catch (const std::exception& e) {
            std::cout << "Execution failed: " << e.what() << "\n";
            continue;
        }

}

return 0;
}
