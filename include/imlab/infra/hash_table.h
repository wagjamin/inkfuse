// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_INFRA_HASH_TABLE_H_
#define INCLUDE_IMLAB_INFRA_HASH_TABLE_H_
//---------------------------------------------------------------------------
#include <atomic>
#include <deque>
#include <algorithm>
#include <utility>
#include <vector>
#include <iterator>
#include <limits>
#include <iostream>
#include "imlab/infra/hash.h"
#include "imlab/infra/bits.h"
#include "imlab/infra/settings.h"
#include "tbb/parallel_for.h"
//---------------------------------------------------------------------------
// template<typename ... T> struct IsKey : std::false_type { };
// template<typename ... T> struct IsKey<Key<T...>> : std::true_type { };
//---------------------------------------------------------------------------
template<typename T> struct Tester;
//---------------------------------------------------------------------------
template <typename KeyT, typename ValueT, typename Hasher = std::hash<KeyT>>
class LazyMultiMap {
    // Check key type
    // static_assert(IsKey<KeyT>::value, "The key of LazyMultiMap must be a Key<T>");

 protected:
    // Entry in the hash table
    struct Entry {
        // Pointer to the next element in the collision list
        // Note: has to be atomic to allow for parallel finalization phase.
        std::atomic<Entry*> next;
        // Key of the hash table entry
        KeyT key;
        // Value of the hash table entry
        ValueT value;

        // Constructor
        Entry(KeyT key, ValueT value)
            : next(nullptr), key(key), value(value) {}
    };

    // EqualRangeIterator for the hash table
    class EqualRangeIterator: public std::iterator<std::forward_iterator_tag, ValueT> {
     public:
        // Default constructor
        EqualRangeIterator(): ptr_(nullptr), key_() {}
        // Constructor
        explicit EqualRangeIterator(Entry *ptr, KeyT key): ptr_(ptr), key_(key) {
            advance();
        }
        // Destructor
        ~EqualRangeIterator() = default;

        // Postfix increment
        EqualRangeIterator operator++(int) {
            EqualRangeIterator result{*this};
            ++this;
            return result;
        }
        // Prefix increment
        EqualRangeIterator &operator++() {
            auto start = ptr_;
            if (ptr_->next) {
                ptr_ = ptr_->next;
            } else {
                ptr_ = nullptr;
            }
            advance();
            assert(start == nullptr || start != ptr_);
            return *this;
        }
        // Reference
        ValueT &operator*() {
            assert(key_ == ptr_->key);
            return ptr_->value;
        }
        // Pointer
        ValueT *operator->() { return &(ptr_->value); }
        // Equality
        bool operator==(const EqualRangeIterator &other) const { return ptr_ == other.ptr_; }
        // Inequality
        bool operator!=(const EqualRangeIterator &other) const { return ptr_ != other.ptr_; }

     protected:
        // Entry pointer
        Entry *ptr_;
        // Key that is being searched for
        KeyT key_;

        void advance() {
            while (ptr_ && !(ptr_->key == key_)) {
                ptr_ = ptr_->next.load();
            }
        }
    };

 public:
    LazyMultiMap() {
        if constexpr (imlab::Settings::USE_TBB) {
            entries_.resize(tbb::this_task_arena::max_concurrency());
        } else {
            entries_.resize(1);
        }
    }

    // End of the equal range
    EqualRangeIterator end() { return EqualRangeIterator(); }

    // Insert an element into the hash table
    //  * Gather all entries with insert and build the hash table with finalize.
    void insert(const KeyT& key, const ValueT& val) {
        if constexpr (imlab::Settings::USE_TBB) {
            // Insert into thread-local DAG.
            auto idx = std::max(0, tbb::this_task_arena::current_thread_index());
            assert(idx <= entries_.size());
            entries_[idx].template emplace_back(key, val);
        } else {
            entries_[0].template emplace_back(key, val);
        }
    }

    // Finalize the hash table
    //  * Get the next power of two, which is larger than the number of entries (include/imlab/infra/bits.h).
    //  * Resize the hash table to that size.
    //  * For each entry in entries_, calculate the hash and prepend it to the collision list in the hash table.
    void finalize() {
        size_ = 0;
        count_ = 0;
        for (auto& elem: entries_) {
            size_ += static_cast<size_t>(1.2 * elem.size());
            count_ += elem.size();
        }
        hash_table_ = std::make_unique<std::atomic<Entry*>[]>(size_);

        // TODO Currently bugy indexing logic, have to fix this at some point.
        // But the overall threading logic within the map is correct I think.
        if constexpr(false || imlab::Settings::USE_TBB) {
            // Remove elements which received no tuples.
            entries_.erase(std::remove_if(
                    entries_.begin(), entries_.end(),
                    [](const auto& x) {
                        return x.size() == 0;
                    }), entries_.end());

            std::vector<size_t> running_sums(entries_.size(), 0);
            for (size_t k = 1; k < entries_.size(); ++k) {
                for (size_t j = k; j < entries_.size(); ++j) {
                    running_sums[j] += entries_[k].size();
                }
            }

            // Multi-threaded building of the hash table
            tbb::parallel_for(tbb::blocked_range<int>(0, count_),
               [&](tbb::blocked_range<int> r)
               {
                    // Find starting offset in running_sums.
                    size_t idx = 0;

                   for (auto it = r.begin(); it != r.end(); ++it)
                   {
                       // Update running sums.
                       while (idx + 1 <= running_sums.size() && running_sums[idx + 1] <= it) {
                           idx++;
                       }

                       int64_t offset = it - running_sums[idx];
                       assert(offset >= 0);
                       if (offset >= entries_[idx].size()) {
                           std::cout << offset << "," << entries_[idx].size() << std::endl;
                       }
                       auto& entry = entries_[idx][offset];
                       size_t hash = hasher(entry.key);
                       auto map_idx = hash % size_;
                       auto old = hash_table_[map_idx].exchange(&entry);
                       entry.next.store(old);
                   }
               });
        } else {
            for (auto& thread_result: entries_) {
                for (auto& entry: thread_result) {
                    size_t hash = hasher(entry.key);
                    auto idx = hash % size_;
                    auto old = hash_table_[idx].exchange(&entry);
                    entry.next.store(old);
                }
            }
        }

    }

    // To find an element, calculate the hash (Key::Hash), and search this list until you reach a nullptr;
    std::pair<EqualRangeIterator, EqualRangeIterator> equal_range(const KeyT& key) {
        auto hash = hasher(key);
        auto idx = hash % size_;
        return std::make_pair(EqualRangeIterator(hash_table_[idx], key), EqualRangeIterator());
    }

 protected:
    // Entries of the hash table. Outer vector is for a thread id.
    std::vector<std::deque<Entry>> entries_;
    // The hash table.
    // Use the next_ pointers in the entries to store the collision list of the hash table.
    //
    //      hash_table_     entries_
    //      +---+           +---+
    //      | * | --------> | x | --+
    //      | 0 |           |   |   |
    //      | 0 |           |   |   |
    //      | 0 |           | z | <-+
    //      +---+           +---+
    //
    std::unique_ptr<std::atomic<Entry*>[]> hash_table_;
    // Size of the above array.
    size_t size_ = 0;
    // How many elements there are in total.
    size_t count_ = 0;
    // Backing hashers.
    Hasher hasher;
    // Note: decided against masking here. No black magic doing it though, just use clz on the vector size and
    // do some bit twiddling.
};
//---------------------------------------------------------------------------
#endif  // INCLUDE_IMLAB_INFRA_HASH_TABLE_H_
//---------------------------------------------------------------------------
