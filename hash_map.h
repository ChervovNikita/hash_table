#include <utility>
#include <vector>
#include <list>
#include <algorithm>
#include <stdexcept>
#include <iostream>

template<typename KeyType, typename ValueType, typename Hash = std::hash<KeyType>> class HashMap {
public:
    using CellType = std::pair<KeyType, ValueType>;
    using ListIterator = typename std::list<CellType>::iterator;
    using ListIteratorConst = typename std::list<CellType>::const_iterator;

    class iterator {
    public:
        iterator() = default;
        explicit iterator(ListIterator iter): iter_(iter) {}
        iterator operator++() {
            ++iter_;
            return *this;
        }
        iterator operator++(int) {
            iterator output = *this;
            ++iter_;
            return output;
        }
        bool operator==(iterator other) {
            return iter_ == other.iter_;
        }
        bool operator!=(iterator other) {
            return iter_ != other.iter_;
        }
        std::pair<const KeyType, ValueType>& operator*() {
            return reinterpret_cast<std::pair<const KeyType, ValueType>&>(*iter_);
        }
        std::pair<const KeyType, ValueType>* operator->() {
            return &reinterpret_cast<std::pair<const KeyType, ValueType>&>(*iter_);
        }
    private:
        ListIterator iter_;
    };

    class const_iterator {
    public:
        const_iterator() = default;
        explicit const_iterator(ListIteratorConst iter): iter_(iter) {}
        const_iterator operator++() {
            ++iter_;
            return *this;
        }
        const_iterator operator++(int) {
            const_iterator output = *this;
            ++iter_;
            return output;
        }
        bool operator==(const_iterator other) {
            return iter_ == other.iter_;
        }
        bool operator!=(const_iterator other) {
            return iter_ != other.iter_;
        }
        const std::pair<const KeyType, ValueType>& operator*() {
            return reinterpret_cast<const std::pair<const KeyType, ValueType>&>(*iter_);
        }
        const std::pair<const KeyType, ValueType>* operator->() {
            return &reinterpret_cast<const std::pair<const KeyType, ValueType>&>(*iter_);
        }
    private:
        ListIteratorConst iter_;
    };

    iterator begin() {
        return iterator(elements.begin());
    }

    iterator end() {
        return iterator(elements.end());
    }

    const_iterator begin() const {
        return const_iterator(elements.cbegin());
    }

    const_iterator end() const {
        return const_iterator(elements.cend());
    }

    iterator find(KeyType query) {
        int pos_ = receive_pos_(query);
        if (pos_ == -1) return iterator(elements.end());
        return iterator(buckets[pos_].iter);
    }

    const_iterator find(KeyType query) const {
        int pos_ = receive_pos_(query);
        if (pos_ == -1) return const_iterator(elements.cend());
        return const_iterator(buckets[pos_].iter);
    }

    explicit HashMap(Hash hash = Hash()): hash_(std::move(hash)) {
        capasity_ = 20;
        elements = std::list<CellType>();
        buckets = std::vector<node>(capasity_);
    }

    template<class T>
    HashMap(T begin, T end, Hash hash = Hash()): hash_(hash) {
        size_t size = 0;
        for (auto element = begin; element != end; ++element) {
            ++size;
        }
        capasity_ = 5 * size + 20;
        buckets = std::vector<node>(capasity_);
        for (auto element = begin; element != end; ++element) {
            insert(*element);
        }
    }

    HashMap(typename std::initializer_list<CellType> pairs,
            Hash hash = Hash()): hash_(hash){
        capasity_ = 5 * pairs.size() + 20;
        buckets = std::vector<node>(capasity_);
        for (auto element = pairs.begin(); element != pairs.end(); ++element) {
            insert(*element);
        }
    }

    size_t size() const {
        return elements.size();
    }

    bool empty() const {
        return !elements.size();
    }

    Hash hash_function() const {
        return hash_;
    }

    void insert(CellType x) {
        if (receive_pos_(x.first) == -1) {
            insert_(x);
            check_and_rehash();
        }
    }

    void erase(const KeyType x) {
        if (receive_pos_(x) != -1) {
            erase_(x);
        }
    }

    ValueType& operator[](const KeyType query) {
        int pos = receive_pos_(query);
        if (pos == -1) {
            CellType to_insert = {query, ValueType()};
            insert_(to_insert);
            check_and_rehash();
            pos = receive_pos_(query);
            return buckets[pos].iter->second;
        }
        check_and_rehash();
        return buckets[pos].iter->second;
    }

    const ValueType& at(const KeyType query) const {
        int pos = receive_pos_(query);
        if (pos == -1) {
            throw std::out_of_range("Out of range error");
        }
        return buckets[pos].iter->second;
    }

    void clear() {
        capasity_ = 20;
        elements.clear();
        buckets = std::vector<node>(capasity_);
    }

    void check_and_rehash() {
        double load_factor = elements.size() / capasity_;
        if (load_factor < 0.5) {
            return;
        }
        capasity_ = 5 * elements.size() + 20;
        auto old_elements = elements;
        elements.clear();
        buckets = std::vector<node>(capasity_);
        for (auto& element : old_elements) {
            insert_(element);
        }
    }

private:
    size_t capasity_ = 20;
    Hash hash_ = Hash();
    std::list<CellType> elements;
    struct node {
        typename std::list<CellType>::iterator iter;
        int waiting = -1;
    };
    std::vector<node> buckets = std::vector<node>(20);

    int receive_pos_(const KeyType& key) const {
        size_t current_id = hash_(key) % capasity_;
        int wait = 0;
        while (buckets[current_id].waiting >= wait) {
            if (key == buckets[current_id].iter->first) {
                return current_id;
            }
            ++wait;
            ++current_id;
            if (current_id == capasity_) {
                current_id = 0;
            }
        }
        return -1;
    }

    void insert_(const CellType& value) {
        elements.push_back(value);
        auto elements_pos = --elements.end();

        size_t current_id = hash_(value.first) % capasity_;
        int current_waiting = 0;
        while (buckets[current_id].waiting != -1) {
            if (buckets[current_id].waiting < current_waiting) {
                std::swap(buckets[current_id].waiting, current_waiting);
                std::swap(buckets[current_id].iter, elements_pos);
            }
            ++current_waiting;
            ++current_id;
            if (current_id == capasity_) {
                current_id = 0;
            }
        }
        buckets[current_id].iter = elements_pos;
        buckets[current_id].waiting = current_waiting;
    }

    void erase_(const KeyType& key) {
        size_t current_id = hash_(key) % capasity_;
        while (buckets[current_id].iter->first != key) {
            ++current_id;
            if (current_id == capasity_) {
                current_id = 0;
            }
        }

        elements.erase(buckets[current_id].iter);
        buckets[current_id].waiting = -1;

        size_t next_id = current_id + 1;
        if (next_id == capasity_) {
            next_id = 0;
        }
        while (buckets[next_id].waiting > 0) {
            std::swap(buckets[current_id], buckets[next_id]);
            --buckets[current_id].waiting;
            current_id = next_id;
            ++next_id;
            if (next_id == capasity_) {
                next_id = 0;
            }
        }
    }
};
