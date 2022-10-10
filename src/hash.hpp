#pragma once

inline u64 hash_data(void* data, u32 len, u64 seed = 14695981039346656037ull) {
    const u8* input = (u8*) data;
    while (len-- > 0)
        seed = 1099511628211ull * (seed ^ *input++);

    return seed;
}