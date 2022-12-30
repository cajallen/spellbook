#pragma once

#include <vector>
#include <span>
using std::span;

namespace spellbook {

template <typename T>
struct vector {
    std::vector<T> internal;

    vector();
    vector(std::initializer_list<T> list);
    explicit vector(const T* begin, const T* end);
    explicit vector(u32 capacity);
    explicit vector(u32 size, const T& t);

    template <typename... Args>
    void emplace_back(Args&&...args);
    void push_back(T&& t);
    void push_back(const T& t);

    template <typename... Args>
    void emplace(u32 i, Args&&...args);
    void insert(u32 i, T&& t);
    void insert(u32 i, const T& t);

    void append(const vector<T>& other);
    template <typename S>
    void append_data(const S& s); 

    template <typename Predicate>
    void remove_if(Predicate pr, bool unordered = true);
    void remove_index(u32 i, bool unordered = true);
    void remove_indices(u32 start, u32 end, bool unordered = true);
    void remove_value(const T& t, bool unordered = true);
    void remove_back();

    void clear();
    void resize(u32 size);
    void rebsize(u32 size);
    void reserve(u32 capacity);

    [[nodiscard]] u32 find(const T& t);
    [[nodiscard]] u32 index(const T& t);

    [[nodiscard]] u32  size() const;
    [[nodiscard]] u32  bsize() const;
    [[nodiscard]] bool empty() const;
    [[nodiscard]] bool contains(const T& t) const;

    [[nodiscard]] T*       data();
    [[nodiscard]] T*       begin();
    [[nodiscard]] T*       end();
    [[nodiscard]] T&       front();
    [[nodiscard]] T&       back();
    [[nodiscard]] const T* data() const;
    [[nodiscard]] const T* begin() const;
    [[nodiscard]] const T* end() const;
    [[nodiscard]] const T& first() const;
    [[nodiscard]] const T& last() const;


    [[nodiscard]] T&       at(u32 index);
    [[nodiscard]] T&       operator[](u32 index);
    [[nodiscard]] const T& at(u32 index) const;
    [[nodiscard]] const T& operator[](u32 index) const;
};

template <typename T>
vector<T>::vector()
    : internal() {
}

template <typename T>
vector<T>::vector(u32 capacity)
    : internal() {
    this->reserve(capacity);
}

template <typename T>
vector<T>::vector(std::initializer_list<T> list)
    : internal(list) {
}

template <typename T>
vector<T>::vector(const T* begin, const T* end)
    : internal(begin, end) {
}

template <typename T>
vector<T>::vector(u32 size, const T& t) 
    : internal(size, t) {
}

template <typename T>
template <typename... Args>
void vector<T>::emplace_back(Args&&... args) {
    internal.emplace_back(std::forward<Args>(args)...);
}

template <typename T>
void vector<T>::push_back(T&& t) {
    internal.push_back(t);
}

template <typename T>
void vector<T>::push_back(const T& t) {
    internal.push_back(t);
}

template <typename T>
template <typename... Args>
void vector<T>::emplace(u32 i, Args&&... args) {
    internal.emplace(internal.begin() + i, std::forward<Args>(args)...);
}

template <typename T>
void vector<T>::insert(u32 i, T&& t) {
    internal.insert(internal.begin() + i, t);
}

template <typename T>
void vector<T>::insert(u32 i, const T& t) {
    internal.insert(internal.begin() + i, t);
}

template <typename T>
void vector<T>::append(const vector<T>& other) {
    internal.insert(internal.end(), other.begin(), other.end());
}

template <>
template <typename S>
void vector<u8>::append_data(const S& s) {
    internal.insert(internal.end(), (u8*) &s, (u8*) &s + sizeof(S));
}


template <typename T>
template <typename Predicate>
void vector<T>::remove_if(Predicate predicate, bool unordered) {
    if (unordered) {
        for (int i = 0; i < this->size();) {
            if (predicate(this->at(i)))
                this->remove_index(i, true);
            else
                i++;
        }
    } else {
        for (int i = 0; i < this->size();) {
            if (predicate(this->at(i)))
                this->remove_index(i, false);
            else
                i++;
        }
    }
}

template <typename T>
void vector<T>::remove_index(u32 i, bool unordered) {
    if (unordered) {
        if (i != (this->size() - 1))
            std::swap(this->at(i), this->back());
        this->remove_back();
    } else {
        internal.erase(internal.begin() + i);
    }
}

template <typename T>
void vector<T>::remove_indices(u32 start, u32 end, bool unordered) {
    if (unordered) {
        for (int i = start; i < end; i++) {
            this->remove_index(i, true);
        }
    } else {
        internal.erase(internal.begin() + start, internal.begin() + end);
    }
}

template <typename T>
void vector<T>::remove_value(const T& t, bool unordered) {
    if (unordered) {
        for (int i = 0; i < this->size();) {
            if (this->at(i) == t)
                this->remove_index(i, true);
            else
                i++;
        }
    } else {
        internal.erase(std::remove(internal.begin(), internal.end(), t), internal.end());
    }
}

template <typename T>
void vector<T>::remove_back() {
    internal.pop_back();
}

template <typename T>
void vector<T>::clear() {
    internal.clear();
}

template <typename T>
void vector<T>::resize(u32 size) {
    internal.resize(size);
}

template <typename T>
void vector<T>::rebsize(u32 size) {
    internal.resize(size / sizeof(T));
}

template <typename T>
void vector<T>::reserve(u32 capacity) {
    internal.reserve(capacity);
}

template <typename T>
u32 vector<T>::find(const T& t) {
    return std::find(this->begin(), this->end(), t) - this->begin();
}

template <typename T>
u32 vector<T>::index(const T& t) {
    if (this->begin() <= &t && &t < this->end())
        return &t - this->begin();
    return -1;
}

template <typename T>
u32 vector<T>::size() const {
    return internal.size();
}

template <typename T>
u32 vector<T>::bsize() const {
    return internal.size() * sizeof(T);
}

template <typename T>
bool vector<T>::empty() const {
    return internal.empty();
}

template <typename T>
bool vector<T>::contains(const T& t) const {
    return std::find(internal.begin(), internal.end(), t) != internal.end();
}

template <typename T>
T* vector<T>::data() {
    return internal.size() > 0 ? internal.data() : nullptr;
}

template <typename T>
T* vector<T>::begin() {
    return this->data();
}

template <typename T>
T* vector<T>::end() {
    return this->data() + this->size();
}

template <typename T>
T& vector<T>::front() {
    return internal.front();
}

template <typename T>
T& vector<T>::back() {
    return internal.back();
}

template <typename T>
const T* vector<T>::data() const {
    return internal.size() > 0 ? internal.data() : nullptr;
}

template <typename T>
const T* vector<T>::begin() const {
    return this->data();
}

template <typename T>
const T* vector<T>::end() const {
    return this->data() + this->size();
}

template <typename T>
const T& vector<T>::first() const {
    return internal.front();
}

template <typename T>
const T& vector<T>::last() const {
    return internal.back();
}

template <typename T>
T& vector<T>::at(u32 index) {
    return internal.at(index);
}

template <typename T>
T& vector<T>::operator[](u32 index) {
    return internal[index];
}

template <typename T>
const T& vector<T>::at(u32 index) const {
    return internal.at(index);
}

template <typename T>
const T& vector<T>::operator[](u32 index) const {
    return internal[index];
}

}


