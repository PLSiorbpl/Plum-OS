#pragma once
#include "vector.hpp"

namespace std {
    template<typename Key, typename Value, typename Hash>
    class hash_map {
    public:
        struct Entry {
            size_t hash;
            Key key;
            Value value;
        };

        // ------------------
        // Constructors
        hash_map() = default;

        hash_map(const hash_map& other) {
            for (size_t i = 0; i < 8; i++)
                buckets[i] = other.buckets[i];
        }

        hash_map& operator=(const hash_map& other) {
            if (this != &other) {
                for (size_t i = 0; i < 8; i++)
                    buckets[i] = other.buckets[i];
            }
            return *this;
        }

        hash_map(hash_map&& other) noexcept {
            for (size_t i = 0; i < 8; i++)
                buckets[i] = std::move(other.buckets[i]);
        }

        hash_map& operator=(hash_map&& other) noexcept {
            if (this != &other) {
                for (size_t i = 0; i < 8; i++)
                    buckets[i] = std::move(other.buckets[i]);
            }
            return *this;
        }

        // ------------------
        // Functions

        void insert(const Key& key, Value&& value) {
            const size_t h = hash_function(key);
            const size_t idx = h % 8;

            if (auto* v = find(key)) {
                *v = std::move(value);
                return;
            }

            buckets[idx].push_back({h, key, std::move(value)});
        }

        void erase(const Key& key) {
            const size_t h = hash_function(key);
            const size_t idx = h % 8;

            auto& bucket = buckets[idx];
            for (size_t i = 0; i < bucket.size(); i++) {
                if (bucket[i].hash == h && bucket[i].key == key) {
                    bucket.swap_remove(i);
                    return;
                }
            }
        }

        Value* find(const Key& key) {
            const size_t h = hash_function(key);
            const size_t idx = h % 8;

            for (auto& entry : buckets[idx]) {
                if (entry.hash == h && entry.key == key)
                    return &entry.value;
            }
            return nullptr;
        }

        const Value* find(const Key& key) const {
            const size_t h = hash_function(key);
            const size_t idx = h % 8;

            for (const auto& entry : buckets[idx]) {
                if (entry.hash == h && entry.key == key)
                    return &entry.value;
            }
            return nullptr;
        }

        bool contains(const Key& key) const {
            return find(key) != nullptr;
        }

        Value& operator[](const Key& key) {
            if (auto* v = find(key))
                return *v;

            insert(key, Value{});
            return *find(key);
        }

        [[nodiscard]] bool empty() const {
            for (const auto& bucket : buckets) {
                if (!bucket.empty())
                    return false;
            }
            return true;
        }

        [[nodiscard]] size_t size() const {
            size_t s = 0;
            for (const auto& bucket : buckets)
                s += bucket.size();
            return s;
        }

        void clear() {
            for (auto& bucket : buckets)
                bucket.clear();
        }

        void reserve(size_t bucket_count) {
            // For now only 8
        }

        // ------------------
        // Iterators

        struct iterator {
            hash_map* map;
            size_t bucket;
            size_t index;

            iterator(hash_map* m, size_t b, size_t i) : map(m), bucket(b), index(i) {
                advance();
            }

            void advance() {
                while (bucket < 8 && index >= map->buckets[bucket].size()) {
                    bucket++;
                    index = 0;
                }
            }

            Entry& operator*() const { return map->buckets[bucket][index]; }
            Entry* operator->() const { return &map->buckets[bucket][index]; }

            iterator& operator++() {
                index++;
                advance();
                return *this;
            }

            bool operator==(const iterator& other) const {
                return bucket == other.bucket && index == other.index;
            }
            bool operator!=(const iterator& other) const { return !(*this == other); }
        };

        struct const_iterator {
            const hash_map* map;
            size_t bucket;
            size_t index;

            const_iterator(const hash_map* m, size_t b, size_t i) : map(m), bucket(b), index(i) {
                advance();
            }

            void advance() {
                while (bucket < 8 && index >= map->buckets[bucket].size()) {
                    bucket++;
                    index = 0;
                }
            }

            const Entry& operator*() const { return map->buckets[bucket][index]; }
            const Entry* operator->() const { return &map->buckets[bucket][index]; }

            const_iterator& operator++() {
                index++;
                advance();
                return *this;
            }

            bool operator==(const const_iterator& other) const {
                return bucket == other.bucket && index == other.index;
            }
            bool operator!=(const const_iterator& other) const { return !(*this == other); }
        };

        iterator begin() { return iterator(this, 0, 0); }
        iterator end() { return iterator(this, 8, 0); }

        const_iterator begin() const { return const_iterator(this, 0, 0); }
        const_iterator end() const { return const_iterator(this, 8, 0); }

        const_iterator cbegin() const { return begin(); }
        const_iterator cend() const { return end(); }

    private:
        Hash hash_function;

        std::vector<Entry> buckets[8];
    };
}
