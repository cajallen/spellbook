#pragma once

#include "umap.hpp"

#include "json.hpp"
#include "console.hpp"
#include "math.hpp"


namespace spellbook {

template <typename T>
struct Archive;

template <typename T>
struct id_ptr {
    u64 value;

    static Archive<T>& archive();
    static id_ptr<T>   null();
    static id_ptr<T>   ptr(T& t);

    id_ptr();
    id_ptr(u64 value);
    id_ptr(const T& t, u64 value);

    template <typename... Args> static id_ptr<T> emplace(Args&&...args);

    bool valid() const;
    void remove() const;

    T& operator*() const;
    T* operator->() const;

    bool operator==(id_ptr<T> other) const;
    bool operator!=(id_ptr<T> other) const;
};

template <typename T>
id_ptr<T> from_jv_impl(const json_value& jv, id_ptr<T>* _) {
    json      j = from_jv<json>(jv);
    id_ptr<T> value(from_jv<T>(*j["node"]), from_jv<u64>(*j["id"]));
    return value;
}

template <typename T>
json_value to_jv(const id_ptr<T>& value) {
    auto j = json();
    j["node"] = make_shared<json_value>(to_jv(*value));
    j["id"]   = make_shared<json_value>(to_jv(value.value)); 
    return to_jv(j);
}

template <typename T>
struct Archive {
    umap<u64, T>  vals;
    umap<T*, u64> ptrs;
};

template <typename T>
Archive<T>& id_ptr<T>::archive() {
    static Archive<T> arch;
    return arch;
}

template <typename T>
id_ptr<T> id_ptr<T>::null() {
    return id_ptr<T>(0);
}

template <typename T>
id_ptr<T> id_ptr<T>::ptr(T& t) {
    auto& arch = archive();
    return id_ptr<T>(arch.ptrs[&t]);
}

template <typename T>
id_ptr<T>::id_ptr(u64 value)
    : value(value) {
}

template <typename T>
id_ptr<T>::id_ptr(const T& t, u64 value)
    : value(value) {
    auto& arch                   = archive();
    arch.vals[value]             = t;
    arch.ptrs[&arch.vals[value]] = value;
}

template <typename T>
id_ptr<T>::id_ptr() : value(0) {
}

template <typename T>
template <typename... Args>
id_ptr<T> id_ptr<T>::emplace(Args&&...args) {
    auto&     arch = archive();
    id_ptr<T> new_id;
    new_id.value                        = math::random_u64();
    arch.vals[new_id.value]             = T(std::forward<Args>(args)...);
    arch.ptrs[&arch.vals[new_id.value]] = new_id.value;
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
    return arch.vals[value];
}

template <typename T>
T* id_ptr<T>::operator->() const {
    assert_else(this->valid());
    auto& arch = archive();
    return &arch.vals[value];
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
