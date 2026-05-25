/**
 * @file ResourceManagerStatic.h
 * @brief Static resource manager with compile-time pool
 */
#ifndef FRAMEWORK_RESOURCE_MANAGER_STATIC_H
#define FRAMEWORK_RESOURCE_MANAGER_STATIC_H

#include <utility>
#include <vector>
#include <cstddef>

template <typename KeyType, typename ValType, size_t NUM>
class ResourceManagerStatic {
public:
    using KeyValueType = std::pair<KeyType, ValType>;

private:
    KeyValueType* __pool_;

    ValType __defaultValue_;

public:
    explicit ResourceManagerStatic(KeyValueType* init) { __pool_ = init; }

    void setDefaultValue(ValType&& defaultValue) {
        __defaultValue_ = { std::forward<ValType>(defaultValue) };
    }

    const ValType& getDefaultValue() { return __defaultValue_; }

    const ValType& get(KeyType key);

    template<typename F>
    void forEach(F&& func) {
        for (size_t i = 0; i < NUM; i++) {
            func(__pool_[i]);
        }
    }
};

#include <algorithm>
#include <cstring>

template <typename KeyType, typename ValType, size_t NUM>
const ValType& ResourceManagerStatic<KeyType, ValType, NUM>::get(KeyType key)
{
    using KeyValType = std::pair<KeyType, ValType>;
    auto res = std::find_if(__pool_, __pool_ + NUM, [&key](KeyValType node) {
        return strcmp(node.first, key) == 0;
    });

    if (res == __pool_ + NUM) {
        return __defaultValue_;
    } else {
        return res->second;
    }
}

#endif  // FRAMEWORK_RESOURCE_MANAGER_STATIC_H