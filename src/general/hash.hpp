#pragma once

#include <tracy/Tracy.hpp>

namespace spellbook {

template <typename T>
constexpr u64 hash_data(const T* data, u32 len, u64 seed = 14695981039346656037ull) {
    auto input = (const u8*) data;
    while (len-- > 0)
        seed = 1099511628211ull * (seed ^ *input++);

    return seed;
}

constexpr u64 hash_string(const string& data, u64 seed = 14695981039346656037ull) {
    return hash_data(data.data(), data.size(), seed);
}

constexpr u64 hash_view(std::string_view data, u64 seed = 14695981039346656037ull) {
    const char* input = data.data();
    u32 len = data.size();
    while (len-- > 0)
        seed = 1099511628211ull * (seed ^ *input++);

    return seed;
}

}
