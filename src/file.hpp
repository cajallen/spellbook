#pragma once

#include "vector.hpp"
#include "string.hpp"

#include "console.hpp"

namespace spellbook {

string get_file(const string& str, bool with_ext = false);
string get_folder(const string& str);
string get_extension(const string& str);
string get_contents(const string& file_name, bool binary = false);
bool   file_exists(const string& file_name);

template <typename T> vector<T> get_contents(const string& file_name, bool binary = true) {
    FILE* f = fopen(file_name.c_str(), !binary ? "r" : "rb");
    fmt_assert_else(f, "Source: {} not found", file_name)
        return vector<T>{};

    fseek(f, 0, SEEK_END);
    size_t    size = ftell(f) / sizeof(T);
    vector<T> contents;
    contents.resize(size);
    rewind(f);

    fread(&contents[0], sizeof(T), size, f);

    fclose(f);

    return contents;
}

}
