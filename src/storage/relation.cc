#include "storage/relation.h"

namespace imlab {

    BaseColumn::BaseColumn(bool nullable_) : nullable(nullable_) {
    }

    bool BaseColumn::isNullable() {
        return nullable;
    }

    BaseColumn &StoredRelation::getColumn(std::string_view name) const {
        for (const auto&[n, c] : columns) {
            if (n == name) {
                return *c;
            }
        }
        throw std::runtime_error("Named column does not exist");
    }

    size_t StoredRelation::getColumnId(std::string_view name) const
    {
        for (auto it = columns.cbegin(); it < columns.cend(); ++it) {
            if (it->first == name) {
                return std::distance(columns.cbegin(), it);
            }
        }
        throw std::runtime_error("Named column does not exist. Cannot get id.");
    }

    std::pair<std::string_view, BaseColumn &> StoredRelation::getColumn(size_t index) const {
        if (index >= columns.size()) {
            throw std::runtime_error("Indexed column does not exist");
        }
        auto &col = columns[index];
        return {col.first, *col.second};
    }

    size_t StoredRelation::columnCount() const {
        return columns.size();
    }

    void StoredRelation::setPrimaryKey(std::vector<size_t> pk_) {
        for (const auto idx : pk) {
            if (idx >= columns.size()) {
                throw std::runtime_error("PK index no in range");
            }
        }
        pk = std::move(pk_);
    }

    void StoredRelation::loadRows(std::istream &stream) {
        std::string line;
        while (stream.peek() != EOF && std::getline(stream, line)) {
            loadRow(line);
            addLastRowToIndex();
        }
    }

    void StoredRelation::loadRow(const std::string &str) {
        size_t currPos = 0;
        for (auto&[_, c] : columns) {
            if (currPos == str.length()) {
                throw std::runtime_error("Not enough columns in TSV");
            }
            size_t end = str.find('|', currPos);
            if (end == std::string::npos) {
                end = str.length();
            }
            c->loadValue(str.data() + currPos, end - currPos);
            currPos = end + 1;
        }
        // Row is active.
        appendRow();
        if (currPos != str.length() + 1) {
            throw std::runtime_error("Too many columns in TSV");
        }
    }

    void StoredRelation::appendRow() {
        tombstones.push_back(true);
    }

    void StoredRelation::dropRow(size_t idx) {
        tombstones[idx] = false;
    }

    size_t StoredRelation::getSize() const {
        return tombstones.size();
    }

} // namespace imlab
