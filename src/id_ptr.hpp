#pragma once

#include "umap.hpp"

#include "json.hpp"
#include "console.hpp"

namespace spellbook {

template<typename T>
struct id_ptr {
    u64 value;

    static umap<id_ptr<T>, T> archive() {
        static umap<id_ptr<T>, T> mapping;
        return mapping;
    }

    T& operator*() const {
        assert_else(*this != null());
        auto& arch = archive();
        if (!arch.contains(value))
            arch[value] = T();
        return arch[value];
    }
    T* operator->() const {
        assert_else(*this != null())
            return nullptr;
        auto& arch = archive();
        if (!arch.contains(value))
            arch[value] = T();
        return &arch[value];
    }

    bool operator==(id_ptr<T> other) const { return value == other.value; }

    static id_ptr<T> null() { return id_ptr<T>{0}; }

    id_ptr() = default;
    JSON_IMPL(id_ptr, value);
};

}