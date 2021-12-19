// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_ALGEBRA_INNER_JOIN_H_
#define INCLUDE_IMLAB_ALGEBRA_INNER_JOIN_H_
// ---------------------------------------------------------------------------
#include <memory>
#include <utility>
#include <vector>
#include "imlab/algebra/iu.h"
#include "imlab/algebra/operator.h"
// ---------------------------------------------------------------------------
namespace imlab {
// ---------------------------------------------------------------------------
class InnerJoin: public Operator {
 protected:
    // Left child operator
    std::unique_ptr<Operator> left_child;
    // Right child operator
    std::unique_ptr<Operator> right_child;
    // Hash predicates
    std::vector<std::pair<const IU*, const IU*>> hash_predicates;

    // Required ius
    std::set<const IU*> required_ius;
    // Consumer
    Operator *consumer;

    // State required within the jitted join. Set up in prepare for later use within the Consume functions.
    struct InnerJoinState {
        InnerJoinState(
                std::set<const IU*> keys_left_,
                std::set<const IU*> values_left_,
                std::set<const IU*> keys_right_
        ): keys_left(std::move(keys_left_)), values_left(std::move(values_left_)), keys_right(std::move(keys_right_)) {};

        // Key tuple type of the hash table.
        std::string keytype_tuple;
        // Key type of the hash table.
        std::string keytype;
        // Value type of the hash table.
        std::string valuetype;
        // Hash table name for this join.
        std::string ht_name;

        // Keys coming from the left (insert).
        std::set<const IU*> keys_left;
        // Values coming from the left (insert).
        std::set<const IU*> values_left;
        // Keys coming from the right (lookup).
        std::set<const IU*> keys_right;

        // Setup work.
        void prepare(CodegenHelper& codegen);

        // Pack iu collection into std::tuple named 'key_name'.
        void packLeftKey(CodegenHelper& codegen, const std::string& key_name);

        // Pack left value into std::tuple named 'value_name'.
        void packLeftValue(CodegenHelper& codegen, const std::string& value_name);

        // Pack right key into std__tuple named 'key_name'.
        void packRightKey(CodegenHelper& codegen, const std::string& key_name);

        // Unpack left value from std::tuple named 'value_name'
        void unpackLeftValue(CodegenHelper& codegen, const std::string& value_name);

    private:
        void packTuple(CodegenHelper& codegen, const std::set<const IU*>& ius, const std::string& tuple_name);
    };

    // Inner join state after prepare.
    std::optional<InnerJoinState> state;

public:
    // Constructor
    InnerJoin(
            CodegenHelper& codegen_helper_,
            std::unique_ptr<Operator> left,
            std::unique_ptr<Operator> right,
            std::vector<std::pair<const IU*, const IU*>>&& predicates)
        : Operator(codegen_helper_), left_child(std::move(left)), right_child(std::move(right)), hash_predicates(predicates) {}

    // Collect all IUs produced by the operator
    std::vector<const IU*> CollectIUs() override;

    // Prepare the operator
    void Prepare(const std::set<const IU*> &required, Operator* consumer) override;
    // Produce all tuples
    void Produce() override;
    // Consume tuple
    void Consume(const Operator* child) override;

private:
    // Consume a tuple from the left side.
    void ConsumeLeft();
    // Consume a tuple from the right side.
    void ConsumeRight();
};
// ---------------------------------------------------------------------------
}  // namespace imlab
// ---------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_ALGEBRA_INNER_JOIN_H_
// ---------------------------------------------------------------------------

