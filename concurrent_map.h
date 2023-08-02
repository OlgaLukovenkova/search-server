#pragma once

#include <random>
#include <string>
#include <vector>
#include <mutex>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count)
        : buckets_(bucket_count) {
    }

    Access operator[](const Key& key) {
        auto& bucket = buckets_[static_cast<size_t>(key) % buckets_.size()];
        return { std::lock_guard( bucket.mutex ), bucket.map[key]};
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& [mutex, map] : buckets_) {
            std::lock_guard g(mutex);
            result.insert(map.begin(), map.end());
        }
        return result;
    }

    void erase(const Key& key) {
        size_t bucket_index = static_cast<size_t>(key) % buckets_.size();
        std::lock_guard guard(buckets_[bucket_index].mutex);
        buckets_[bucket_index].map.erase(key);
    }

private:
    struct Bucket {
        std::mutex mutex;
        std::map<Key, Value> map;
    };

    std::vector<Bucket> buckets_;
};
