#include "file.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace spellbook {

string get_file(const string& str, bool with_ext) {
    size_t found = str.find_last_of("/\\");
    size_t end   = with_ext ? str.size() : str.find_last_of(".");
    return str.substr(found + 1);
}

string get_folder(const string& str) {
    size_t found = str.find_last_of("/\\");
    return str.substr(0, found);
}

string get_extension(const string& str) {
    size_t found = str.find_last_of(".");
    return str.substr(found + 1);
}

string get_contents(const string& file_name, bool binary) {
    FILE* f = fopen(file_name.c_str(), !binary ? "r" : "rb");
    if (f == nullptr)
        return {};
    
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f) / sizeof(char);
    string contents;
    contents.resize(size);
    rewind(f);

    size_t read_bytes = fread(contents.data(), sizeof(char), size, f);

    fclose(f);

    contents.resize(std::min(strlen(contents.data()), read_bytes));
    return contents;
}

vector<u32> get_contents_u32(const string& file_name, bool binary) {
    FILE* f = fopen(file_name.c_str(), !binary ? "r" : "rb");
    if (f == nullptr)
        return {};
    
    fseek(f, 0, SEEK_END);
    size_t    size = ftell(f) / sizeof(u32);
    vector<u32> contents;
    contents.resize(size);
    rewind(f);

    fread(&contents[0], sizeof(u32), size, f);

    fclose(f);

    return contents;
}

bool file_exists(const string& file_name) {
    return fs::exists(file_name);
}


}
