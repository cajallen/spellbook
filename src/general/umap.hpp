#pragma once

#include <robin_hood.h>

namespace spellbook {

template <typename Key, typename T, typename Hash = robin_hood::hash<Key>, typename KeyEqual = std::equal_to<Key>,
    size_t MaxLoadFactor100 = 80>
using umap = robin_hood::detail::Table<false, MaxLoadFactor100, Key, T, Hash, KeyEqual>;

template <typename Key, typename Hash = robin_hood::hash<Key>, typename KeyEqual = std::equal_to<Key>, size_t MaxLoadFactor100 = 80>
using uset = robin_hood::detail::Table<false, MaxLoadFactor100, Key, void, Hash, KeyEqual>;

}