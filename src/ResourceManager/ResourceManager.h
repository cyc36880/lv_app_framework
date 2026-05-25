/**
 * @file ResourceManager.h
 * @brief Resource manager for key-value pair lookup
 */
#ifndef FRAMEWORK_RESOURCE_MANAGER_H
#define FRAMEWORK_RESOURCE_MANAGER_H

#include <stddef.h>

template <typename KEY_TYPE, typename VALUE_TYPE>
class ResourceManager {
public:
    typedef struct {
        KEY_TYPE key;
        VALUE_TYPE value;
    } KEY_PAIR;

    typedef bool (*COMPARE_CALLBACK)(KEY_TYPE key1, KEY_TYPE key2);

public:
    ResourceManager(KEY_PAIR* array, size_t size, COMPARE_CALLBACK compareCallback, VALUE_TYPE defaultValue)
        : _array(array), _size(size), _compareCallback(compareCallback), _defaultValue(defaultValue) {}

    ~ResourceManager() {}

    VALUE_TYPE* get(KEY_TYPE key) {
        for (size_t i = 0; i < _size; i++) {
            if (_compareCallback(key, _array[i].key)) {
                return &_array[i].value;
            }
        }
        return &_defaultValue;
    }

    bool set(KEY_TYPE key, VALUE_TYPE value) {
        for (size_t i = 0; i < _size; i++) {
            if (_compareCallback(key, _array[i].key)) {
                _array[i].value = value;
                return true;
            }
        }
        return false;
    }

    void setDefault(VALUE_TYPE value) { _defaultValue = value; }
    VALUE_TYPE getDefault() { return _defaultValue; }

private:
    KEY_PAIR* _array;
    size_t _size;
    COMPARE_CALLBACK _compareCallback;
    VALUE_TYPE _defaultValue;
};

#endif  // FRAMEWORK_RESOURCE_MANAGER_H