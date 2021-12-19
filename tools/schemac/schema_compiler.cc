// ---------------------------------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------------------------------
#include "imlab/schemac/schema_compiler.h"
#include "imlab/schemac/schema_parse_context.h"
#include <sstream>
// ---------------------------------------------------------------------------------------------------
namespace imlab {
// ---------------------------------------------------------------------------------------------------
namespace schemac {
// ---------------------------------------------------------------------------------------------------
namespace {
// ---------------------------------------------------------------------------------------------------
static const std::string SPACE = "    ";
// ---------------------------------------------------------------------------------------------------
} // namespace
// ---------------------------------------------------------------------------------------------------
// Compile a schema
void SchemaCompiler::Compile(Schema &schema)
{
    // First we write the header.
    writePreeamble(schema);
    // Create the backing structs.
    writeRelationStruct(schema);
    // And implement the actual CRUD functions.
    writeCRUD(schema);
    // Finally we write the footer.
    writePostamble(schema);
}
// ---------------------------------------------------------------------------------------------------
void SchemaCompiler::writeRelationStruct(const Schema &schema)
{
    // TODO(Benjamin): Add full index support.
    for (const auto& table: schema.tables) {
        auto struct_name = "table_" + table.id;
        // H file.
        header << "struct " << struct_name << " final : public StoredRelation {\n";
        header << "public:\n";

        header << "~" << struct_name << "() override = default;\n";

        // Backing tuple type for a full row ordered by create table declaration.
        header << "using TupleType = std::tuple<\n";
        for (auto it = table.columns.cbegin(); it < table.columns.cend(); ++it) {
            // Attach column.
            if (std::next(it) != table.columns.cend()) {
                header << SPACE << std::string(it->type.Desc()) << ",\n";
            } else {
                header << SPACE << std::string(it->type.Desc()) << "\n";
            }
        }
        header << ">;\n";

        if (!table.primary_key.empty()) {
            header << "using PKType = std::tuple<\n";
            for (auto it = table.primary_key.cbegin(); it != table.primary_key.cend(); ++it) {
                header << SPACE << table.getType(*it).Desc();
                // Note that std::set gives us deterministic iteration order.
                if (std::next(it) != table.primary_key.cend()) {
                    header << ",";
                }
                header << "\n";
            }
            header << ">;\n";
        }

        header << "table_" << table.id << "();\n";

        // Create new Row.
        header << "void Create(\n";
        header << SPACE << "const TupleType& newRow\n";
        header << ");\n";

        // Read a row based on a TID.
        header << "TupleType Read(\n";
        header << SPACE << "size_t tid\n";
        header << ");\n";

        // Update TID based on new values.
        header << "void Update(\n";
        header << SPACE << "size_t tid,\n";
        header << SPACE << "const TupleType& newRow\n";
        header << ");\n";

        // Delete TID.
        header << "void Delete(\n";
        header << SPACE << "size_t tid\n";
        header << ");\n";

        // Add last row to the index.
        header << "virtual void addLastRowToIndex(\n";
        header << ") override;\n";

        if (!table.primary_key.empty()) {
            // Lookup TID.
            header << "size_t Lookup(\n";
            header << SPACE << "const PKType& key\n";
            header << ");\n";

            header << R"(

// Custom, non standard compatible iterator.
struct KeyIterator
{
public:
    // Create [start, end[ iterator into the the relation.
)";

            header << "KeyIterator(" << struct_name << "& table_, PKType start_, PKType end_): table(table_), it_start(table.pk_map.lower_bound(start_)), it_end(table.pk_map.upper_bound(end_)) {\n";
            header << "}\n\n";

            header << "std::optional<TupleType> next();\n";

            header << "private:\n";
            // Backing iterator to the beginning.
            header << struct_name << "& table;\n";
            header << "std::map<PKType, size_t>::iterator it_start;\n";
            header << "std::map<PKType, size_t>::iterator it_end;\n";
            header << "};\n\n";

            // Assemble the PK from a given row.
            header << "static PKType Assemble(TupleType row);\n\n";
        }

        header << "private:\n";

        if (!table.primary_key.empty()) {
            header << "std::map<PKType, size_t> pk_map;\n\n";
        }

        header << "};\n\n";

        // CC file.
        impl << struct_name << "::" << struct_name << "() {\n";

        for (const auto& column: table.columns) {
            // Attach column.
            impl << SPACE << "attachTypedColumn<" << std::string(column.type.Desc()) << ">(\"" +  column.id + "\");\n";
        }

        impl << "}\n\n";
    }

}
// ---------------------------------------------------------------------------------------------------
void SchemaCompiler::writeCRUD(const Schema &schema)
{
    for (const auto& table: schema.tables) {
        auto struct_name = "table_" + table.id;
        auto tuple_type = struct_name + "::TupleType";

        // Create new Row.
        impl << "void " << struct_name << "::Create(\n";
        impl << SPACE << "const TupleType& newRow\n";
        impl<< ") {\n";
        for (auto it = table.columns.cbegin(); it != table.columns.cend(); ++it) {
            auto idx = std::to_string(std::distance(table.columns.cbegin(), it));
            // Get backing column.
            impl << SPACE << "{\n";
            // Get backing column.
            impl << SPACE << SPACE << "auto& col = static_cast<TypedColumn<" << it->type.Desc();
            impl << ">&>(getColumn(" << idx  << ").second);\n";

            // Write the value.
            impl << SPACE << SPACE << "col.getStorage().push_back(std::get<" << idx << ">(newRow));\n";

            impl << SPACE << "}\n";
        }
        if (!table.primary_key.empty()) {
            impl << SPACE << "auto pk = Assemble(newRow);\n";
            impl << SPACE << "auto tid = getSize() - 1;\n";
            impl << SPACE << "pk_map[pk] = tid;\n";
        }
        impl << SPACE << "appendRow();\n";
        impl<< "}\n\n";

        // Read a row based on a TID.
        impl << tuple_type << " " << struct_name << "::Read(\n";
        impl << SPACE << "size_t tid\n";
        impl<< ") {\n";
        impl << SPACE << "TupleType result;\n";
        for (auto it = table.columns.cbegin(); it != table.columns.cend(); ++it) {
            impl << SPACE << "{\n";

            // Get the backing column.
            auto dist = std::distance(table.columns.cbegin(), it);
            impl << SPACE << SPACE << "auto& col = static_cast<TypedColumn<" << it->type.Desc();
            impl << ">&>(getColumn(" << dist << ").second);\n";

            // Read the value.
            auto idx = std::to_string(std::distance(table.columns.cbegin(), it));
            impl << SPACE << SPACE <<  "std::get<" << idx << ">(result) = col.getStorage()[tid];\n";

            impl << SPACE << "}\n";
        }
        // Return the assembled tuple.
        impl << SPACE << "return result;\n";
        impl << "}\n\n";

        // Update a given row based on the tid.
        impl << "void " << struct_name << "::Update(\n";
        impl << "size_t tid,\n";
        impl << SPACE << "const TupleType& newRow\n";
        impl << ") {\n";
        if (!table.primary_key.empty()) {
            // Erase old key.
            impl << SPACE << "auto row = Read(tid);\n";
            impl << SPACE << "auto pk = Assemble(row);\n";
            impl << SPACE << "pk_map.erase(pk);\n";
            // And insert new one.
            impl << SPACE << "auto pk_new = Assemble(newRow);\n";
            impl << SPACE << "pk_map[pk_new] = tid;\n";
        }
        for (auto it = table.columns.cbegin(); it != table.columns.cend(); ++it) {
            impl << SPACE << "{\n";
            // Get backing column.
            auto dist = std::distance(table.columns.cbegin(), it);
            impl << SPACE << SPACE << "auto& col = static_cast<TypedColumn<" << it->type.Desc();
            impl << ">&>(getColumn(" << dist << ").second);\n";

            // Write the value.
            auto idx = std::to_string(std::distance(table.columns.cbegin(), it));
            impl << SPACE << SPACE << "col.getStorage()[tid] = (std::get<" << idx << ">(newRow));\n";

            impl << SPACE << "}\n";
        }
        impl<< "}\n\n";

        // Update a given row based on the tid.
        impl << "void " << struct_name << "::Delete(\n";
        impl << SPACE << "size_t tid\n";
        impl << ") {\n";
        if (!table.primary_key.empty()) {
            impl << SPACE << "auto row = Read(tid);\n";
            impl << SPACE << "auto pk = Assemble(row);\n";
            impl << SPACE << "pk_map.erase(pk);\n";
        }
        impl << SPACE << "dropRow(tid);\n";
        impl << "}\n\n";

        // Update a given row based on the tid.
        impl << "void " << struct_name << "::addLastRowToIndex(\n";
        impl << ") {\n";
        if (!table.primary_key.empty()) {
            impl << SPACE << "auto tid = getSize() - 1;";
            impl << SPACE << "auto row = Read(tid);\n";
            impl << SPACE << "auto pk = Assemble(row);\n";
            impl << SPACE << "pk_map[pk] = tid;\n";
        }
        impl << "}\n\n";

        if (!table.primary_key.empty()) {
            // Lookup TID.
            impl << "size_t " << struct_name << "::Lookup(\n";
            impl << SPACE << "const PKType& key\n";
            impl << ") {\n";
            impl << SPACE << "return pk_map[key];\n";
            impl << "}\n\n";

            // Assemble PK sub-row from row.
            impl << struct_name << "::PKType " << struct_name << "::Assemble(\n";
            impl << SPACE << "TupleType row\n";
            impl << ") {\n";
            impl << SPACE << "PKType result;\n";
            for (auto it = table.primary_key.cbegin(); it != table.primary_key.cend(); ++it) {
                auto pk_idx = std::to_string(std::distance(table.primary_key.cbegin(), it));
                auto backing_idx = std::to_string(table.getPKIndex(*it));

                impl << SPACE << "std::get<" << pk_idx << ">(result) = std::get<" << backing_idx << ">(row);\n";
            }
            impl << SPACE << "return result;\n";
            impl << "}\n\n";

            impl << "std::optional<" << struct_name << "::TupleType> " << struct_name << "::KeyIterator::next() {\n";
            impl << SPACE << "if (it_start != it_end) {\n";
            // Build tuple.
            impl << SPACE << SPACE << "TupleType result;\n";
            impl << SPACE << SPACE << "size_t tid = it_start->second;\n";
            for (auto it = table.columns.cbegin(); it != table.columns.cend(); ++it) {
                auto dist = std::distance(table.columns.cbegin(), it);
                impl << SPACE << SPACE << "{\n";
                // Get backing column.
                impl << SPACE << SPACE << SPACE << "auto& col = static_cast<TypedColumn<" << it->type.Desc();
                impl << ">&>(table.getColumn(" << dist << ").second);\n";

                // Insert value.
                impl << SPACE << SPACE << SPACE << "std::get<" << dist << ">(result) = col.getStorage()[tid];\n";

                impl << SPACE << SPACE << "}\n";
            }
            impl << SPACE << SPACE << "it_start++;\n";
            impl << SPACE << SPACE << "return result;\n";
            impl << SPACE << "}\n";
            // No more tuples.
            impl << SPACE << "return std::nullopt;\n";
            impl << "}\n\n";
        }

    }

}
// ---------------------------------------------------------------------------------------------------
void SchemaCompiler::writePreeamble(const Schema &schema)
{
    header << R"(
#pragma once

#include "imlab/storage/relation.h"
#include "imlab/infra/types.h"
#include <tuple>
#include <map>
#include <cstddef>
#include <iterator>

namespace imlab {

)";

    impl << "#include \"" + header_path + "\"\n";
    impl << "namespace imlab {\n";
}
// ---------------------------------------------------------------------------------------------------
void SchemaCompiler::writePostamble(const Schema &schema)
{
    header << R"(

} // namespace imlab

)";

    impl << "} // namespace imlab\n";
}
// ---------------------------------------------------------------------------------------------------
} // namespace schemac
// ---------------------------------------------------------------------------------------------------
} // namespace imlab
// ---------------------------------------------------------------------------------------------------
