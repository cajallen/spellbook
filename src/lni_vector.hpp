#pragma once

#include <stdexcept>

#define LNI_VECTOR_MAX_SZ 1000000000

namespace lni {

template <typename T>
class vector {
public:
    // types:
    typedef T                                     value_type;
    typedef T&                                    reference;
    typedef const T&                              const_reference;
    typedef T*                                    pos32er;
    typedef const T*                              const_pos32er;
    typedef T*                                    iterator;
    typedef const T*                              const_iterator;
    typedef std::reverse_iterator<iterator>       reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    typedef ptrdiff_t                             difference_type;
    typedef u32                          size_type;

    // 23.3.11.2, construct/copy/destroy:
    vector() noexcept;
    explicit vector(size_type n);
    vector(size_type n, const T& val);
    template <class InputIt> vector(InputIt first, InputIt last); //v1(v2.begin(),v2.end())
    vector(std::initializer_list<T>);
    vector(const vector<T>&);
    vector(vector<T>&&) noexcept;
    ~vector();
    vector<T>&                    operator =(const vector<T>&);
    vector<T>&                    operator =(vector<T>&&);
    vector<T>&                    operator =(std::initializer_list<T>);
    void                          assign(size_type, const T& value);
    template <class InputIt> void assign(InputIt, InputIt);
    void                          assign(std::initializer_list<T>);

    // iterators:
    iterator               begin() noexcept;
    const_iterator         cbegin() const noexcept;
    iterator               end() noexcept;
    const_iterator         cend() const noexcept;
    reverse_iterator       rbegin() noexcept;
    const_reverse_iterator crbegin() const noexcept;
    reverse_iterator       rend() noexcept;
    const_reverse_iterator crend() const noexcept;

    // 23.3.11.3, capacity:
    bool      empty() const noexcept;
    size_type size() const noexcept;
    size_type max_size() const noexcept;
    size_type capacity() const noexcept;
    void      resize(size_type);
    void      resize(size_type, const T&);
    void      reserve(size_type);
    void      shrink_to_fit();

    // element access
    reference       operator [](size_type);
    const_reference operator [](size_type) const;
    reference       at(size_type);
    const_reference at(size_type) const;
    reference       front();
    const_reference front() const;
    reference       back();
    const_reference back() const;

    // 23.3.11.4, data access:
    T*       data() noexcept;
    const T* data() const noexcept;

    // 23.3.11.5, modifiers:
    template <class ... Args> void emplace_back(Args&& ...args);
    void                           push_back(const T&);
    void                           push_back(T&&);
    void                           pop_back();

    template <class ... Args> iterator emplace(const_iterator, Args&& ...);
    iterator                           insert(const_iterator, const T&);
    iterator                           insert(const_iterator, T&&);
    iterator                           insert(const_iterator, size_type, const T&);
    template <class InputIt> iterator  insert(const_iterator, InputIt, InputIt);
    iterator                           insert(const_iterator, std::initializer_list<T>);
    iterator                           erase(const_iterator);
    iterator                           erase(const_iterator, const_iterator);
    void                               swap(vector<T>&);
    void                               clear() noexcept;

    bool operator ==(const vector<T>&) const;
    bool operator !=(const vector<T>&) const;
    bool operator <(const vector<T>&) const;
    bool operator <=(const vector<T>&) const;
    bool operator >(const vector<T>&) const;
    bool operator >=(const vector<T>&) const;

private:
    size_type rsrv_sz = 4;
    size_type vec_sz  = 0;
    T*        arr;

    inline void reallocate();

};


template <typename T>
vector<T>::vector() noexcept {
    arr = new T[rsrv_sz];
}

template <typename T>
vector<T>::vector(typename vector<T>::size_type n) {
    size_type i;
    rsrv_sz = n << 2;
    arr     = new T[rsrv_sz];
    for (i     = 0; i < n; ++i)
        arr[i] = T();
    vec_sz = n;
}

template <typename T>
vector<T>::vector(typename vector<T>::size_type n, const T& value) {
    size_type i;
    rsrv_sz = n << 2;
    arr     = new T[rsrv_sz];
    for (i     = 0; i < n; ++i)
        arr[i] = value;
    vec_sz = n;
}

template <typename T>
template <class InputIt>
vector<T>::vector(InputIt first, InputIt last) {
    size_type i, count = last - first;
    rsrv_sz            = count << 2;
    vec_sz             = count;
    arr                = new T[rsrv_sz];
    for (i     = 0; i < count; ++i, ++first)
        arr[i] = *first;
}

template <typename T>
vector<T>::vector(std::initializer_list<T> lst) {
    rsrv_sz = lst.size() << 2;
    arr     = new T[rsrv_sz];
    for (auto& item : lst)
        arr[vec_sz++] = item;
}

template <typename T>
vector<T>::vector(const vector<T>& other) {
    size_type i;
    rsrv_sz = other.rsrv_sz;
    arr     = new T[rsrv_sz];
    for (i     = 0; i < other.vec_sz; ++i)
        arr[i] = other.arr[i];
    vec_sz = other.vec_sz;
}

template <typename T>
vector<T>::vector(vector<T>&& other) noexcept {
    size_type i;
    rsrv_sz = other.rsrv_sz;
    arr     = new T[rsrv_sz];
    for (i     = 0; i < other.vec_sz; ++i)
        arr[i] = std::move(other.arr[i]);
    vec_sz = other.vec_sz;
}

template <typename T>
vector<T>::~vector() {
    delete [] arr;
}

template <typename T>
vector<T>& vector<T>::operator =(const vector<T>& other) {

    if (this == &other) {
        return *this;
    }
    size_type i;
    if (rsrv_sz < other.vec_sz) {
        rsrv_sz = other.vec_sz << 2;
        reallocate();
    }
    for (i     = 0; i < other.vec_sz; ++i)
        arr[i] = other.arr[i];
    vec_sz = other.vec_sz;
    return *this;
}

template <typename T>
vector<T>& vector<T>::operator =(vector<T>&& other) {
    size_type i;
    if (rsrv_sz < other.vec_sz) {
        rsrv_sz = other.vec_sz << 2;
        reallocate();
    }
    for (i     = 0; i < other.vec_sz; ++i)
        arr[i] = std::move(other.arr[i]);
    vec_sz = other.vec_sz;
    return *this;
}

template <typename T>
vector<T>& vector<T>::operator =(std::initializer_list<T> lst) {
    if (rsrv_sz < lst.size()) {
        rsrv_sz = lst.size() << 2;
        reallocate();
    }
    vec_sz = 0;
    for (auto& item : lst)
        arr[vec_sz++] = item;
    return *this;
}

template <typename T>
void vector<T>::assign(typename vector<T>::size_type count, const T& value) {
    size_type i;
    if (count > rsrv_sz) {
        rsrv_sz = count << 2;
        reallocate();
    }
    for (i     = 0; i < count; ++i)
        arr[i] = value;
    vec_sz = count;
}

template <typename T>
template <class InputIt>
void vector<T>::assign(InputIt first, InputIt last) {
    size_type i, count = last - first;
    if (count > rsrv_sz) {
        rsrv_sz = count << 2;
        reallocate();
    }
    for (i     = 0; i < count; ++i, ++first)
        arr[i] = *first;
    vec_sz = count;
}

template <typename T>
void vector<T>::assign(std::initializer_list<T> lst) {
    size_type i, count = lst.size();
    if (count > rsrv_sz) {
        rsrv_sz = count << 2;
        reallocate();
    }
    i = 0;
    for (auto& item : lst)
        arr[i++] = item;
}


template <typename T>
typename vector<T>::iterator vector<T>::begin() noexcept {
    return arr;
}

template <typename T>
typename vector<T>::const_iterator vector<T>::cbegin() const noexcept {
    return arr;
}

template <typename T>
typename vector<T>::iterator vector<T>::end() noexcept {
    return arr + vec_sz;
}

template <typename T>
typename vector<T>::const_iterator vector<T>::cend() const noexcept {
    return arr + vec_sz;
}

template <typename T>
typename vector<T>::reverse_iterator vector<T>::rbegin() noexcept {
    return reverse_iterator(arr + vec_sz);
}

template <typename T>
typename vector<T>::const_reverse_iterator vector<T>::crbegin() const noexcept {
    return reverse_iterator(arr + vec_sz);
}

template <typename T>
typename vector<T>::reverse_iterator vector<T>::rend() noexcept {
    return reverse_iterator(arr);
}

template <typename T>
typename vector<T>::const_reverse_iterator vector<T>::crend() const noexcept {
    return reverse_iterator(arr);
}


template <typename T>
inline void vector<T>::reallocate() {
    T* tarr = new T[rsrv_sz];
    memcpy(tarr, arr, vec_sz * sizeof(T));
    delete [] arr;
    arr = tarr;
}


template <typename T>
bool vector<T>::empty() const noexcept {
    return vec_sz == 0;
}
template <typename T>
typename vector<T>::size_type vector<T>::size() const noexcept {
    return vec_sz;
}

template <typename T>
typename vector<T>::size_type vector<T>::max_size() const noexcept {
    return LNI_VECTOR_MAX_SZ;
}

template <typename T>
typename vector<T>::size_type vector<T>::capacity() const noexcept {
    return rsrv_sz;
}

template <typename T>
void vector<T>::resize(typename vector<T>::size_type sz) {
    if (sz > vec_sz) {
        if (sz > rsrv_sz) {
            rsrv_sz = sz;
            reallocate();
        }
    } else {
        size_type i;
        for (i = vec_sz; i < sz; ++i)
            arr[i].~T();
    }
    vec_sz = sz;
}

template <typename T>
void vector<T>::resize(typename vector<T>::size_type sz, const T& c) {
    if (sz > vec_sz) {
        if (sz > rsrv_sz) {
            rsrv_sz = sz;
            reallocate();
        }
        size_type i;
        for (i     = vec_sz; i < sz; ++i)
            arr[i] = c;
    } else {
        size_type i;
        for (i = vec_sz; i < sz; ++i)
            arr[i].~T();
    }
    vec_sz = sz;
}

template <typename T>
void vector<T>::reserve(typename vector<T>::size_type _sz) {
    if (_sz > rsrv_sz) {
        rsrv_sz = _sz;
        reallocate();
    }
}

template <typename T>
void vector<T>::shrink_to_fit() {
    rsrv_sz = vec_sz;
    reallocate();
}


template <typename T>
typename vector<T>::reference vector<T>::operator [](typename vector<T>::size_type idx) {
    return arr[idx];
}

template <typename T>
typename vector<T>::const_reference vector<T>::operator [](typename vector<T>::size_type idx) const {
    return arr[idx];
}

template <typename T>
typename vector<T>::reference vector<T>::at(size_type pos) {
    if (pos < vec_sz)
        return arr[pos];
    else
        throw std::out_of_range("accessed position is out of range");
}

template <typename T>
typename vector<T>::const_reference vector<T>::at(size_type pos) const {
    if (pos < vec_sz)
        return arr[pos];
    else
        throw std::out_of_range("accessed position is out of range");
}

template <typename T>
typename vector<T>::reference vector<T>::front() {
    return arr[0];
}

template <typename T>
typename vector<T>::const_reference vector<T>::front() const {
    return arr[0];
}

template <typename T>
typename vector<T>::reference vector<T>::back() {
    return arr[vec_sz - 1];
}

template <typename T>
typename vector<T>::const_reference vector<T>::back() const {
    return arr[vec_sz - 1];
}


template <typename T>
T* vector<T>::data() noexcept {
    return arr;
}

template <typename T>
const T* vector<T>::data() const noexcept {
    return arr;
}


template <typename T>
template <class ... Args>
void vector<T>::emplace_back(Args&& ...args) {
    if (vec_sz == rsrv_sz) {
        rsrv_sz <<= 2;
        reallocate();
    }
    arr[vec_sz] = std::move(T(std::forward<Args>(args) ...));
    ++vec_sz;
}

template <typename T>
void vector<T>::push_back(const T& val) {
    if (vec_sz == rsrv_sz) {
        rsrv_sz <<= 2;
        reallocate();
    }
    arr[vec_sz] = val;
    ++vec_sz;
}

template <typename T>
void vector<T>::push_back(T&& val) {
    if (vec_sz == rsrv_sz) {
        rsrv_sz <<= 2;
        reallocate();
    }
    arr[vec_sz] = std::move(val);
    ++vec_sz;
}

template <typename T>
void vector<T>::pop_back() {
    --vec_sz;
    arr[vec_sz].~T();
}


template <typename T>
template <class ... Args>
typename vector<T>::iterator vector<T>::emplace(typename vector<T>::const_iterator it, Args&& ...args) {
    iterator iit = &arr[it - arr];
    if (vec_sz == rsrv_sz) {
        rsrv_sz <<= 2;
        reallocate();
    }
    memmove(iit + 1, iit, (vec_sz - (it - arr)) * sizeof(T));
    (*iit) = std::move(T(std::forward<Args>(args) ...));
    ++vec_sz;
    return iit;
}

template <typename T>
typename vector<T>::iterator vector<T>::insert(typename vector<T>::const_iterator it, const T& val) {
    iterator iit = &arr[it - arr];
    if (vec_sz == rsrv_sz) {
        rsrv_sz <<= 2;
        reallocate();
    }
    memmove(iit + 1, iit, (vec_sz - (it - arr)) * sizeof(T));
    (*iit) = val;
    ++vec_sz;
    return iit;
}

template <typename T>
typename vector<T>::iterator vector<T>::insert(typename vector<T>::const_iterator it, T&& val) {
    iterator iit = &arr[it - arr];
    if (vec_sz == rsrv_sz) {
        rsrv_sz <<= 2;
        reallocate();
    }
    memmove(iit + 1, iit, (vec_sz - (it - arr)) * sizeof(T));
    (*iit) = std::move(val);
    ++vec_sz;
    return iit;
}

template <typename T>
typename vector<T>::iterator vector<T>::insert(typename vector<T>::const_iterator it, vector<T>::size_type cnt, const T& val) {
    iterator f = &arr[it - arr];
    if (!cnt)
        return f;
    if (vec_sz + cnt > rsrv_sz) {
        rsrv_sz = (vec_sz + cnt) << 2;
        reallocate();
    }
    memmove(f + cnt, f, (vec_sz - (it - arr)) * sizeof(T));
    vec_sz += cnt;
    for (iterator it = f; cnt--; ++it)
        (*it)        = val;
    return f;
}

template <typename T>
template <class InputIt>
typename vector<T>::iterator vector<T>::insert(typename vector<T>::const_iterator it, InputIt first, InputIt last) {
    iterator  f   = &arr[it - arr];
    size_type cnt = last - first;
    if (!cnt)
        return f;
    if (vec_sz + cnt > rsrv_sz) {
        rsrv_sz = (vec_sz + cnt) << 2;
        reallocate();
    }
    memmove(f + cnt, f, (vec_sz - (it - arr)) * sizeof(T));
    for (iterator it = f; first != last; ++it, ++first)
        (*it)        = *first;
    vec_sz += cnt;
    return f;
}

template <typename T>
typename vector<T>::iterator vector<T>::insert(typename vector<T>::const_iterator it, std::initializer_list<T> lst) {
    size_type cnt = lst.size();
    iterator  f   = &arr[it - arr];
    if (!cnt)
        return f;
    if (vec_sz + cnt > rsrv_sz) {
        rsrv_sz = (vec_sz + cnt) << 2;
        reallocate();
    }
    memmove(f + cnt, f, (vec_sz - (it - arr)) * sizeof(T));
    iterator iit = f;
    for (auto& item : lst) {
        (*iit) = item;
        ++iit;
    }
    vec_sz += cnt;
    return f;
}

template <typename T>
typename vector<T>::iterator vector<T>::erase(typename vector<T>::const_iterator it) {
    iterator iit = &arr[it - arr];
    (*iit).~T();
    memmove(iit, iit + 1, (vec_sz - (it - arr) - 1) * sizeof(T));
    --vec_sz;
    return iit;
}

template <typename T>
typename vector<T>::iterator vector<T>::erase(typename vector<T>::const_iterator first, vector<T>::const_iterator last) {
    iterator f = &arr[first - arr];
    if (first == last)
        return f;
    for (; first != last; ++first)
        (*first).~T();
    memmove(f, last, (vec_sz - (last - arr)) * sizeof(T));
    vec_sz -= last - first;
    return f;
}

template <typename T>
void vector<T>::swap(vector<T>& rhs) {
    size_t tvec_sz  = vec_sz,
           trsrv_sz = rsrv_sz;
    T* tarr         = arr;

    vec_sz  = rhs.vec_sz;
    rsrv_sz = rhs.rsrv_sz;
    arr     = rhs.arr;

    rhs.vec_sz  = tvec_sz;
    rhs.rsrv_sz = trsrv_sz;
    rhs.arr     = tarr;
}

template <typename T>
void vector<T>::clear() noexcept {
    size_type i;
    for (i = 0; i < vec_sz; ++i)
        arr[i].~T();
    vec_sz = 0;
}


template <typename T>
bool vector<T>::operator ==(const vector<T>& rhs) const {
    if (vec_sz != rhs.vec_sz)
        return false;
    size_type i;
    for (i = 0; i < vec_sz; ++i)
        if (arr[i] != rhs.arr[i])
            return false;
    return true;
}

template <typename T>
bool vector<T>::operator !=(const vector<T>& rhs) const {
    if (vec_sz != rhs.vec_sz)
        return true;
    size_type i;
    for (i = 0; i < vec_sz; ++i)
        if (arr[i] != rhs.arr[i])
            return true;
    return false;
}

template <typename T>
bool vector<T>::operator <(const vector<T>& rhs) const {
    size_type i, j, ub = vec_sz < rhs.vec_sz ? vec_sz : rhs.vec_sz;
    for (i = 0; i < ub; ++i)
        if (arr[i] != rhs.arr[i])
            return arr[i] < rhs.arr[i];
    return vec_sz < rhs.vec_sz;
}

template <typename T>
bool vector<T>::operator <=(const vector<T>& rhs) const {
    size_type i, j, ub = vec_sz < rhs.vec_sz ? vec_sz : rhs.vec_sz;
    for (i = 0; i < ub; ++i)
        if (arr[i] != rhs.arr[i])
            return arr[i] < rhs.arr[i];
    return vec_sz <= rhs.vec_sz;
}

template <typename T>
bool vector<T>::operator >(const vector<T>& rhs) const {
    size_type i, j, ub = vec_sz < rhs.vec_sz ? vec_sz : rhs.vec_sz;
    for (i = 0; i < ub; ++i)
        if (arr[i] != rhs.arr[i])
            return arr[i] > rhs.arr[i];
    return vec_sz > rhs.vec_sz;
}

template <typename T>
bool vector<T>::operator >=(const vector<T>& rhs) const {
    size_type i, j, ub = vec_sz < rhs.vec_sz ? vec_sz : rhs.vec_sz;
    for (i = 0; i < ub; ++i)
        if (arr[i] != rhs.arr[i])
            return arr[i] > rhs.arr[i];
    return vec_sz >= rhs.vec_sz;
}


template <>
inline void vector<bool>::resize(typename vector<bool>::size_type sz) {
    if (sz > rsrv_sz) {
        rsrv_sz = sz;
        reallocate();
    }
    vec_sz = sz;
}

template <>
inline void vector<s8>::resize(typename vector<s8>::size_type sz) {
    if (sz > rsrv_sz) {
        rsrv_sz = sz;
        reallocate();
    }
    vec_sz = sz;
}

template <>
inline void vector<u8>::resize(typename vector<u8>::size_type sz) {
    if (sz > rsrv_sz) {
        rsrv_sz = sz;
        reallocate();
    }
    vec_sz = sz;
}

template <>
inline void vector<s16>::resize(typename vector<s16>::size_type sz) {
    if (sz > rsrv_sz) {
        rsrv_sz = sz;
        reallocate();
    }
    vec_sz = sz;
}

template <>
inline void vector<u16>::resize(typename vector<u16>::size_type sz) {
    if (sz > rsrv_sz) {
        rsrv_sz = sz;
        reallocate();
    }
    vec_sz = sz;
}

template <>
inline void vector<s32>::resize(typename vector<s32>::size_type sz) {
    if (sz > rsrv_sz) {
        rsrv_sz = sz;
        reallocate();
    }
    vec_sz = sz;
}

template <>
inline void vector<u32>::resize(typename vector<u32>::size_type sz) {
    if (sz > rsrv_sz) {
        rsrv_sz = sz;
        reallocate();
    }
    vec_sz = sz;
}

template <>
inline void vector<s64>::resize(typename vector<s64>::size_type sz) {
    if (sz > rsrv_sz) {
        rsrv_sz = sz;
        reallocate();
    }
    vec_sz = sz;
}

template <>
inline void vector<u64>::resize(typename vector<u64>::size_type sz) {
    if (sz > rsrv_sz) {
        rsrv_sz = sz;
        reallocate();
    }
    vec_sz = sz;
}

template <>
inline void vector<f32>::resize(typename vector<f32>::size_type sz) {
    if (sz > rsrv_sz) {
        rsrv_sz = sz;
        reallocate();
    }
    vec_sz = sz;
}

template <>
inline void vector<f64>::resize(typename vector<f64>::size_type sz) {
    if (sz > rsrv_sz) {
        rsrv_sz = sz;
        reallocate();
    }
    vec_sz = sz;
}

template <>
inline void vector<f128>::resize(typename vector<f128>::size_type sz) {
    if (sz > rsrv_sz) {
        rsrv_sz = sz;
        reallocate();
    }
    vec_sz = sz;
}


template <>
inline void vector<bool>::resize(typename vector<bool>::size_type sz, const bool& c) {
    if (sz > vec_sz) {
        if (sz > rsrv_sz) {
            rsrv_sz = sz;
            reallocate();
        }
        size_type i;
        for (i     = vec_sz; i < sz; ++i)
            arr[i] = c;
    }
    vec_sz = sz;
}

template <>
inline void vector<s8>::resize(typename vector<s8>::size_type sz, const s8& c) {
    if (sz > vec_sz) {
        if (sz > rsrv_sz) {
            rsrv_sz = sz;
            reallocate();
        }
        size_type i;
        for (i     = vec_sz; i < sz; ++i)
            arr[i] = c;
    }
    vec_sz = sz;
}

template <>
inline void vector<u8>::resize(typename vector<u8>::size_type sz, const u8& c) {
    if (sz > vec_sz) {
        if (sz > rsrv_sz) {
            rsrv_sz = sz;
            reallocate();
        }
        size_type i;
        for (i     = vec_sz; i < sz; ++i)
            arr[i] = c;
    }
    vec_sz = sz;
}

template <>
inline void vector<s16>::resize(typename vector<s16>::size_type sz, const s16& c) {
    if (sz > vec_sz) {
        if (sz > rsrv_sz) {
            rsrv_sz = sz;
            reallocate();
        }
        size_type i;
        for (i     = vec_sz; i < sz; ++i)
            arr[i] = c;
    }
    vec_sz = sz;
}

template <>
inline void vector<u16>::resize(typename vector<u16>::size_type sz, const u16& c) {
    if (sz > vec_sz) {
        if (sz > rsrv_sz) {
            rsrv_sz = sz;
            reallocate();
        }
        size_type i;
        for (i     = vec_sz; i < sz; ++i)
            arr[i] = c;
    }
    vec_sz = sz;
}

template <>
inline void vector<s32>::resize(typename vector<s32>::size_type sz, const s32& c) {
    if (sz > vec_sz) {
        if (sz > rsrv_sz) {
            rsrv_sz = sz;
            reallocate();
        }
        size_type i;
        for (i     = vec_sz; i < sz; ++i)
            arr[i] = c;
    }
    vec_sz = sz;
}

template <>
inline void vector<u32>::resize(typename vector<u32>::size_type sz, const u32& c) {
    if (sz > vec_sz) {
        if (sz > rsrv_sz) {
            rsrv_sz = sz;
            reallocate();
        }
        size_type i;
        for (i     = vec_sz; i < sz; ++i)
            arr[i] = c;
    }
    vec_sz = sz;
}

template <>
inline void vector<s64>::resize(typename vector<s64>::size_type sz, const s64& c) {
    if (sz > vec_sz) {
        if (sz > rsrv_sz) {
            rsrv_sz = sz;
            reallocate();
        }
        size_type i;
        for (i     = vec_sz; i < sz; ++i)
            arr[i] = c;
    }
    vec_sz = sz;
}

template <>
inline void vector<u64>::resize(typename vector<u64>::size_type sz, const u64& c) {
    if (sz > vec_sz) {
        if (sz > rsrv_sz) {
            rsrv_sz = sz;
            reallocate();
        }
        size_type i;
        for (i     = vec_sz; i < sz; ++i)
            arr[i] = c;
    }
    vec_sz = sz;
}

template <>
inline void vector<f32>::resize(typename vector<f32>::size_type sz, const f32& c) {
    if (sz > vec_sz) {
        if (sz > rsrv_sz) {
            rsrv_sz = sz;
            reallocate();
        }
        size_type i;
        for (i     = vec_sz; i < sz; ++i)
            arr[i] = c;
    }
    vec_sz = sz;
}

template <>
inline void vector<f64>::resize(typename vector<f64>::size_type sz, const f64& c) {
    if (sz > vec_sz) {
        if (sz > rsrv_sz) {
            rsrv_sz = sz;
            reallocate();
        }
        size_type i;
        for (i     = vec_sz; i < sz; ++i)
            arr[i] = c;
    }
    vec_sz = sz;
}

template <>
inline void vector<f128>::resize(typename vector<f128>::size_type sz, const f128& c) {
    if (sz > vec_sz) {
        if (sz > rsrv_sz) {
            rsrv_sz = sz;
            reallocate();
        }
        size_type i;
        for (i     = vec_sz; i < sz; ++i)
            arr[i] = c;
    }
    vec_sz = sz;
}


template <>
inline void vector<bool>::pop_back() {
    --vec_sz;
}

template <>
inline void vector<s8>::pop_back() {
    --vec_sz;
}

template <>
inline void vector<u8>::pop_back() {
    --vec_sz;
}

template <>
inline void vector<s16>::pop_back() {
    --vec_sz;
}

template <>
inline void vector<u16>::pop_back() {
    --vec_sz;
}

template <>
inline void vector<s32>::pop_back() {
    --vec_sz;
}

template <>
inline void vector<u32>::pop_back() {
    --vec_sz;
}

template <>
inline void vector<s64>::pop_back() {
    --vec_sz;
}

template <>
inline void vector<u64>::pop_back() {
    --vec_sz;
}

template <>
inline void vector<f32>::pop_back() {
    --vec_sz;
}

template <>
inline void vector<f64>::pop_back() {
    --vec_sz;
}

template <>
inline void vector<f128>::pop_back() {
    --vec_sz;
}


template <>
inline vector<bool>::iterator vector<bool>::erase(typename vector<bool>::const_iterator it) {
    iterator iit = &arr[it - arr];
    memmove(iit, iit + 1, (vec_sz - (it - arr) - 1) * sizeof(bool));
    --vec_sz;
    return iit;
}

template <>
inline vector<s8>::iterator vector<s8>::erase(typename vector<s8>::const_iterator it) {
    iterator iit = &arr[it - arr];
    memmove(iit, iit + 1, (vec_sz - (it - arr) - 1) * sizeof(s8));
    --vec_sz;
    return iit;
}

template <>
inline vector<u8>::iterator vector<u8>::erase(typename vector<u8>::const_iterator it) {
    iterator iit = &arr[it - arr];
    memmove(iit, iit + 1, (vec_sz - (it - arr) - 1) * sizeof(u8));
    --vec_sz;
    return iit;
}

template <>
inline vector<s16>::iterator vector<s16>::erase(typename vector<s16>::const_iterator it) {
    iterator iit = &arr[it - arr];
    memmove(iit, iit + 1, (vec_sz - (it - arr) - 1) * sizeof(s16));
    --vec_sz;
    return iit;
}

template <>
inline vector<u16>::iterator vector<u16>::erase(typename vector<u16>::const_iterator it) {
    iterator iit = &arr[it - arr];
    memmove(iit, iit + 1, (vec_sz - (it - arr) - 1) * sizeof(u16));
    --vec_sz;
    return iit;
}

template <>
inline vector<s32>::iterator vector<s32>::erase(typename vector<s32>::const_iterator it) {
    iterator iit = &arr[it - arr];
    memmove(iit, iit + 1, (vec_sz - (it - arr) - 1) * sizeof(s32));
    --vec_sz;
    return iit;
}

template <>
inline vector<u32>::iterator vector<u32>::erase(typename vector<u32>::const_iterator it) {
    iterator iit = &arr[it - arr];
    memmove(iit, iit + 1, (vec_sz - (it - arr) - 1) * sizeof(u32));
    --vec_sz;
    return iit;
}

template <>
inline vector<s64>::iterator vector<s64>::erase(typename vector<s64>::const_iterator it) {
    iterator iit = &arr[it - arr];
    memmove(iit, iit + 1, (vec_sz - (it - arr) - 1) * sizeof(s64));
    --vec_sz;
    return iit;
}

template <>
inline vector<u64>::iterator vector<u64>::erase(
    vector<u64>::const_iterator it) {
    iterator iit = &arr[it - arr];
    memmove(iit, iit + 1, (vec_sz - (it - arr) - 1) * sizeof(u64));
    --vec_sz;
    return iit;
}

template <>
inline vector<f32>::iterator vector<f32>::erase(typename vector<f32>::const_iterator it) {
    iterator iit = &arr[it - arr];
    memmove(iit, iit + 1, (vec_sz - (it - arr) - 1) * sizeof(f32));
    --vec_sz;
    return iit;
}

template <>
inline vector<f64>::iterator vector<f64>::erase(typename vector<f64>::const_iterator it) {
    iterator iit = &arr[it - arr];
    memmove(iit, iit + 1, (vec_sz - (it - arr) - 1) * sizeof(f64));
    --vec_sz;
    return iit;
}

template <>
inline vector<f128>::iterator vector<f128>::erase(typename vector<f128>::const_iterator it) {
    iterator iit = &arr[it - arr];
    memmove(iit, iit + 1, (vec_sz - (it - arr) - 1) * sizeof(f128));
    --vec_sz;
    return iit;
}


template <>
inline vector<bool>::iterator vector<
    bool>::erase(typename vector<bool>::const_iterator first, vector<bool>::const_iterator last) {
    iterator f = &arr[first - arr];
    if (first == last)
        return f;
    memmove(f, last, (vec_sz - (last - arr)) * sizeof(bool));
    vec_sz -= last - first;
    return f;
}

template <>
inline vector<s8>::iterator vector<s8>::erase(typename vector<s8>::const_iterator first,
    vector<s8>::const_iterator                                                               last) {
    iterator f = &arr[first - arr];
    if (first == last)
        return f;
    memmove(f, last, (vec_sz - (last - arr)) * sizeof(s8));
    vec_sz -= last - first;
    return f;
}

template <>
inline vector<u8>::iterator vector<u8>::erase(typename vector<u8>::const_iterator first,
    vector<u8>::const_iterator                                                                   last) {
    iterator f = &arr[first - arr];
    if (first == last)
        return f;
    memmove(f, last, (vec_sz - (last - arr)) * sizeof(u8));
    vec_sz -= last - first;
    return f;
}

template <>
inline vector<s16>::iterator vector<s16>::erase(typename vector<s16>::const_iterator first,
    vector<s16>::const_iterator                                                           last) {
    iterator f = &arr[first - arr];
    if (first == last)
        return f;
    memmove(f, last, (vec_sz - (last - arr)) * sizeof(s16));
    vec_sz -= last - first;
    return f;
}

template <>
inline vector<u16>::iterator vector<u16>::erase(typename vector<u16>::const_iterator first,
    vector<u16>::const_iterator                                                                             last) {
    iterator f = &arr[first - arr];
    if (first == last)
        return f;
    memmove(f, last, (vec_sz - (last - arr)) * sizeof(u16));
    vec_sz -= last - first;
    return f;
}

template <>
inline vector<s32>::iterator vector<s32>::erase(typename vector<s32>::const_iterator first, vector<s32>::const_iterator last) {
    iterator f = &arr[first - arr];
    if (first == last)
        return f;
    memmove(f, last, (vec_sz - (last - arr)) * sizeof(s32));
    vec_sz -= last - first;
    return f;
}

template <>
inline vector<u32>::iterator vector<u32>::erase(typename vector<u32>::const_iterator first,
    vector<u32>::const_iterator                                                                 last) {
    iterator f = &arr[first - arr];
    if (first == last)
        return f;
    memmove(f, last, (vec_sz - (last - arr)) * sizeof(u32));
    vec_sz -= last - first;
    return f;
}

template <>
inline vector<s64>::iterator vector<s64>::erase(typename vector<s64>::const_iterator first,
    vector<s64>::const_iterator                                                                   last) {
    iterator f = &arr[first - arr];
    if (first == last)
        return f;
    memmove(f, last, (vec_sz - (last - arr)) * sizeof(s64));
    vec_sz -= last - first;
    return f;
}

template <>
inline vector<u64>::iterator vector<u64>::erase(
    vector<u64>::const_iterator first, vector<u64>::const_iterator last) {
    iterator f = &arr[first - arr];
    if (first == last)
        return f;
    memmove(f, last, (vec_sz - (last - arr)) * sizeof(u64));
    vec_sz -= last - first;
    return f;
}

template <>
inline vector<f32>::iterator vector<f32>::erase(typename vector<f32>::const_iterator first,
    vector<f32>::const_iterator                                                   last) {
    iterator f = &arr[first - arr];
    if (first == last)
        return f;
    memmove(f, last, (vec_sz - (last - arr)) * sizeof(f32));
    vec_sz -= last - first;
    return f;
}

template <>
inline vector<f64>::iterator vector<f64>::erase(typename vector<f64>::const_iterator first,
    vector<f64>::const_iterator                                                     last) {
    iterator f = &arr[first - arr];
    if (first == last)
        return f;
    memmove(f, last, (vec_sz - (last - arr)) * sizeof(f64));
    vec_sz -= last - first;
    return f;
}

template <>
inline vector<f128>::iterator vector<f128>::erase(typename vector<f128>::const_iterator first,
    vector<f128>::const_iterator                                                               last) {
    iterator f = &arr[first - arr];
    if (first == last)
        return f;
    memmove(f, last, (vec_sz - (last - arr)) * sizeof(f128));
    vec_sz -= last - first;
    return f;
}


template <>
inline void vector<bool>::clear() noexcept {
    vec_sz = 0;
}

template <>
inline void vector<s8>::clear() noexcept {
    vec_sz = 0;
}

template <>
inline void vector<u8>::clear() noexcept {
    vec_sz = 0;
}

template <>
inline void vector<s16>::clear() noexcept {
    vec_sz = 0;
}

template <>
inline void vector<u16>::clear() noexcept {
    vec_sz = 0;
}

template <>
inline void vector<s32>::clear() noexcept {
    vec_sz = 0;
}

template <>
inline void vector<u32>::clear() noexcept {
    vec_sz = 0;
}

template <>
inline void vector<s64>::clear() noexcept {
    vec_sz = 0;
}

template <>
inline void vector<u64>::clear() noexcept {
    vec_sz = 0;
}

template <>
inline void vector<f32>::clear() noexcept {
    vec_sz = 0;
}

template <>
inline void vector<f64>::clear() noexcept {
    vec_sz = 0;
}

template <>
inline void vector<f128>::clear() noexcept {
    vec_sz = 0;
}

}