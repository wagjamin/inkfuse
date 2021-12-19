#include "imlab/algebra/print.h"
#include <sstream>
// ---------------------------------------------------------------------------
namespace imlab {
// ---------------------------------------------------------------------------
std::vector<const IU *> Print::CollectIUs()
{
    // No tuples flow past the print operator.
    return {};
}
// ---------------------------------------------------------------------------
void Print::Prepare(const std::set<const IU *> &required, Operator *consumer_)
{
    consumer = consumer_;
    child->Prepare(required_ius, this);

    // Create operator mutex.
    std::stringstream mutex_stream;
    mutex_stream << "std::mutex print_mutex_" << this;
    codegen_helper.Stmt() << mutex_stream.str();

}
// ---------------------------------------------------------------------------
void Print::Produce()
{
    child->Produce();
}
// ---------------------------------------------------------------------------
void Print::Consume(const Operator *child_)
{
    std::stringstream lock_stream;
    lock_stream << "std::lock_guard<std::mutex> lock_" << this << "(print_mutex_" << this << ")";
    codegen_helper.Stmt() << lock_stream.str();
    codegen_helper.Stmt() << "std::stringstream print_stream";
    for (auto it = required_ius.cbegin(); it != required_ius.cend(); it++) {
        auto stmt = codegen_helper.Stmt();
        stmt << "print_stream << " << (*it)->Varname();
        if (std::next(it) != required_ius.cend()) {
            stmt << " << \',\'";
        } else {
            stmt << " << std::endl";
        }
    }
    codegen_helper.Stmt() << "std::cout << print_stream.str()";
}
// ---------------------------------------------------------------------------
} // namespace imlab
// ---------------------------------------------------------------------------
