#pragma once

namespace spellbook {

template <typename T>
u64 hash_data(T* data, u32 len, u64 seed = 14695981039346656037ull) {
    const u8* input = (u8*) data;
    while (len-- > 0)
        seed = 1099511628211ull * (seed ^ *input++);

    return seed;
}

}