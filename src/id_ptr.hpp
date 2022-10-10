#pragma once

#include "umap.hpp"

#include "json.hpp"
#include "console.hpp"
#include "math.hpp"


namespace spellbook {

template <typename T>
struct archive;

template <typename T>
struct id_ptr {
    u64 value;

    static archive<T> archive();
    static id_ptr<T>  null();
    static id_ptr<T>  ptr(T& t);

    id_ptr();
    id_ptr(u64 value);
    template <typename... Args> id_ptr(Args&&... args);

    bool valid() const;
    void remove() const;

    T& operator*() const;
    T* operator->() const;

    bool operator==(id_ptr<T> other) const;
    bool operator!=(id_ptr<T> other) const;

    JSON_IMPL(id_ptr, value);
};

template <typename T>
struct archive {
    umap<u64, T>  vals;
    umap<T&, u64> ptrs;
};

template <typename T>
archive<T> id_ptr<T>::archive() {
    static umap<id_ptr<T>, T> mapping;
    return mapping;
}

template <typename T>
id_ptr<T> id_ptr<T>::null() {
    return id_ptr<T>(0);
}

template <typename T>
id_ptr<T> id_ptr<T>::ptr(T& t) {
    auto& arch = archive();
    return id_ptr<T>(arch.ptrs[t]);
}

template <typename T>
id_ptr<T>::id_ptr(u64 value) : value(value) {
}

template <typename T>
id_ptr<T>::id_ptr() {
    auto& arch                  = archive();
    value                       = math::random_u64();
    arch.vals[value]            = T();
    arch.ptrs[arch.vals[value]] = value;
}

template <typename T>
template <typename... Args>
id_ptr<T>::id_ptr(Args&&... args) {
    auto& arch                  = archive();
    value                       = math::random_u64();
    arch.vals[value]            = T(args);
    arch.ptrs[arch.vals[value]] = value;
}

template <typename T>
void id_ptr<T>::remove() const {
    auto& arch = archive();
    arch.ptrs.erase(arch.ptr.vals[value]);
    arch.vals.erase(value);
}

template <typename T>
bool id_ptr<T>::valid() const {
    auto& arch = archive();
    if (*this == null())
        return false;
    return arch.vals.contains(value);
}

template <typename T>
T& id_ptr<T>::operator*() const {
    assert_else(this->valid());
    auto& arch = archive();
    return arch[value];
}

template <typename T>
T* id_ptr<T>::operator->() const {
    assert_else(this->valid());
    auto& arch = archive();
    return &arch[value];
}

template <typename T>
bool id_ptr<T>::operator==(id_ptr<T> other) const {
    return value == other.value;
}

template <typename T>
bool id_ptr<T>::operator!=(id_ptr<T> other) const {
    return value != other.value;
}

}
