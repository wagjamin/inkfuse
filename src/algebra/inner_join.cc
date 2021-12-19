#include <imlab/algebra/inner_join.h>
#include <imlab/infra/settings.h>
#include <algorithm>
#include <sstream>
// ---------------------------------------------------------------------------
namespace imlab {
// ---------------------------------------------------------------------------
std::vector<const IU *> InnerJoin::CollectIUs()
{
    auto left = left_child->CollectIUs();
    auto right = right_child->CollectIUs();
    left.insert(left.end(), right.begin(), right.end());
    return left;
}
// ---------------------------------------------------------------------------
void InnerJoin::Prepare(const std::set<const IU *> &required, Operator *consumer_)
{
    // Add predicate IUs to the local set.
    required_ius = required;
    consumer = consumer_;
    std::set<const IU*> required_result = required;
    for (const auto& [iu_1, iu_2]: hash_predicates) {
        required_result.emplace(iu_1);
        required_result.emplace(iu_2);
    }
    left_child->Prepare(required_result, this);
    right_child->Prepare(required_result, this);

    // Get the left value IUs to prepare the hash state for the inner join.
    auto left_ius = left_child->CollectIUs();
    std::set<const IU*> left_ius_set(left_ius.begin(), left_ius.end());
    std::set<const IU*> values_left;
    std::set_intersection(required_ius.begin(), required_ius.end(), left_ius_set.begin(), left_ius_set.end(),
                          std::inserter(values_left, values_left.end()));

    // Get the keys prepare the hash state for the inner join.
    std::set<const IU*> keys_left;
    std::set<const IU*> keys_right;
    for (const auto& [iu_1, iu_2]: hash_predicates) {
        keys_left.emplace(iu_1);
        keys_right.emplace(iu_2);
    }

    // Setup join state.
    state.emplace(keys_left, values_left, keys_right);
    // Prepare hash state.
    state->prepare(codegen_helper);
}
// ---------------------------------------------------------------------------
void InnerJoin::InnerJoinState::prepare(CodegenHelper &codegen)
{
    // Prepare key type.
    std::stringstream key_types_list_stream;
    for (auto key = keys_left.cbegin(); key != keys_left.cend(); key++) {
        key_types_list_stream << (*key)->type.Desc();
        if (std::next(key) != keys_left.cend()) {
            key_types_list_stream << ",";
        }
    }
    std::string key_types_list = key_types_list_stream.str();

    std::stringstream keytype_stream;
    // Key type allows hashing while tuple allows more convenience.
    keytype = "Key<" + key_types_list + ">";
    keytype_tuple = "std::tuple<" + key_types_list + ">";

    // Prepare value type.
    std::stringstream valuetype_stream;
    valuetype_stream << "std::tuple<";
    for (auto value = values_left.cbegin(); value != values_left.cend(); value++) {
        valuetype_stream << (*value)->type.Desc();
        if (std::next(value) != values_left.cend()) {
            valuetype_stream << ",";
        }
    }
    valuetype_stream << ">";
    valuetype = valuetype_stream.str();

    // Prepare hash table.
    std::stringstream ht_name_stream;
    ht_name_stream << "ht_" << this;
    ht_name = ht_name_stream.str();
    if (Settings::CODEGEN_LAZY_MULTIMAP) {
        codegen.Stmt() << "LazyMultiMap<" << keytype << ", " << valuetype << "> " << ht_name;
    } else {
        codegen.Stmt() << "std::unordered_multimap<" << keytype << ", " << valuetype << "> " << ht_name;
    }
}
// ---------------------------------------------------------------------------
void InnerJoin::InnerJoinState::packLeftKey(CodegenHelper &codegen, const std::string &key_name)
{
    packTuple(codegen, keys_left, key_name);
}
// ---------------------------------------------------------------------------
void InnerJoin::InnerJoinState::packLeftValue(CodegenHelper &codegen, const std::string &value_name)
{
    packTuple(codegen, values_left, value_name);
}
// ---------------------------------------------------------------------------
void InnerJoin::InnerJoinState::packRightKey(CodegenHelper &codegen, const std::string &key_name)
{
    packTuple(codegen, keys_right, key_name);
}
// ---------------------------------------------------------------------------
void InnerJoin::InnerJoinState::unpackLeftValue(CodegenHelper &codegen, const std::string &value_name)
{
    for (auto iu = values_left.cbegin(); iu != values_left.cend(); iu++) {
        auto idx = std::distance(values_left.cbegin(), iu);
        std::stringstream unpack_stream;
        // Note that we have to declare the type here as we are the ones "creating" this IU.
        unpack_stream << (*iu)->type.Desc() << " " << (*iu)->Varname();
        unpack_stream << " = std::get<" << idx << ">(" << value_name << ")";
        codegen.Stmt() << unpack_stream.str();
    }
}
// ---------------------------------------------------------------------------
void InnerJoin::InnerJoinState::packTuple(CodegenHelper &codegen, const std::set<const IU *>& ius, const std::string& tuple_name)
{
    for (auto iu = ius.cbegin(); iu != ius.cend(); iu++) {
        auto idx = std::distance(ius.cbegin(), iu);
        std::stringstream pack_stream;
        pack_stream << "std::get<" << idx << ">(" << tuple_name << ")";
        pack_stream << " = " << (*iu)->Varname();
        codegen.Stmt() << pack_stream.str();
    }
}
// ---------------------------------------------------------------------------
void InnerJoin::Produce()
{
    // Produce tuples from left side.
    left_child->Produce();
    if constexpr (Settings::CODEGEN_LAZY_MULTIMAP) {
        // Finalize hash table. Commented out as we use simple ordered map atm.
        codegen_helper.Stmt() << state->ht_name << ".finalize()";
    }
    // Produce tuples from right side.
    right_child->Produce();
}
// ---------------------------------------------------------------------------
void InnerJoin::Consume(const Operator *child)
{
    if (child == left_child.get()) {
        ConsumeLeft();
    } else {
        ConsumeRight();
    }
}
// ---------------------------------------------------------------------------
void InnerJoin::ConsumeLeft()
{
    // Pack key.
    std::stringstream packed_key_stream;
    packed_key_stream << "packed_key_" << this;
    std::string packed_key = packed_key_stream.str();
    // Declare packed key.
    codegen_helper.Stmt() << state->keytype << " " << packed_key;
    // Pack it into the components of the actual key.
    state->packLeftKey(codegen_helper, packed_key + ".components");

    // Pack value.
    std::stringstream packed_value_stream;
    packed_value_stream << "packed_value_" << this;
    std::string packed_value = packed_value_stream.str();
    // Declare packed value.
    codegen_helper.Stmt() << state->valuetype << " " << packed_value;
    // Pack it.
    state->packLeftValue(codegen_helper, packed_value);

    // Insert packed tuples into hash table.
    std::stringstream ht_insert_stmt;
    if constexpr (Settings::CODEGEN_LAZY_MULTIMAP) {
        ht_insert_stmt << state->ht_name << ".insert(";
        ht_insert_stmt << packed_key << ", ";
        ht_insert_stmt << packed_value << ")";
    } else {
        ht_insert_stmt << state->ht_name << "[";
        ht_insert_stmt << packed_key << "] =";
        ht_insert_stmt << packed_value;
    }
    codegen_helper.Stmt() << ht_insert_stmt.str();
}
// ---------------------------------------------------------------------------
void InnerJoin::ConsumeRight()
{
    // Pack key.
    std::stringstream packed_key_stream;
    packed_key_stream << " packed_key_right_" << this;
    std::string packed_key = packed_key_stream.str();
    // Declare packed key.
    codegen_helper.Stmt() << state->keytype << " " << packed_key;
    // Pack it into the components of the actual key.
    state->packRightKey(codegen_helper, packed_key + ".components");

    // Get iterators into packed right key.
    std::stringstream get_iterators_stmt;
    get_iterators_stmt << "auto [it_start_" << this << ", ";
    get_iterators_stmt << "it_end_" << this << "] = " ;
    get_iterators_stmt << state->ht_name << ".equal_range(";
    get_iterators_stmt << packed_key << ")";
    codegen_helper.Stmt() << get_iterators_stmt.str();
    // Setup for loop.
    std::stringstream for_loop;
    for_loop << "for(;";
    for_loop << "it_start_" << this << " != it_end_" << this << ";";
    for_loop << "++it_start_" << this << ")";
    codegen_helper.Flow() << for_loop.str();
    {
        // Start new scoped block.
        auto block = codegen_helper.CreateScopedBlock();
        // Read current tuple from iterator.
        std::stringstream packed_value_stream;
        packed_value_stream << "packed_tuple_read_" << this;
        std::string packed_value = packed_value_stream.str();
        std::stringstream tuple_from_ht_stream;
        if constexpr (Settings::CODEGEN_LAZY_MULTIMAP) {
            tuple_from_ht_stream << state->valuetype << "& " << packed_value <<  " = *it_start_" << this;
        } else {
            tuple_from_ht_stream << state->valuetype << "& " << packed_value <<  " = it_start_" << this << "->second";
        }
        codegen_helper.Stmt() << tuple_from_ht_stream.str();
        // Unpack tuple into IUs.
        state->unpackLeftValue(codegen_helper, packed_value);
        consumer->Consume(this);
    }
}
// ---------------------------------------------------------------------------
} // namespace imlab
// ---------------------------------------------------------------------------
