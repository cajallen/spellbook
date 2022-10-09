
#include "asset_loader.hpp"

#include <fstream>
#include <iostream>

namespace spellbook {

bool save_binary_file(const string& path, const AssetFile& input_file) {
    std::ofstream outfile;
    outfile.open(path, std::ios::binary | std::ios::out);
    if (!outfile.is_open()) {
        std::cerr << "Error when trying to write file: " << path << std::endl;
        return false;
    }
    const u32 version     = input_file.version;
    const u32 json_length = (u32) input_file.json.size();
    const u32 blob_length = (u32) input_file.binary_blob.size();
    outfile.write(input_file.type, 4);
    outfile.write((const char*) &version, sizeof(u32));
    outfile.write((const char*) &json_length, sizeof(u32));
    outfile.write((const char*) &blob_length, sizeof(u32));
    outfile.write(input_file.json.data(), json_length);
    outfile.write((const char*) input_file.binary_blob.data(), input_file.binary_blob.size());
    outfile.close();

    return true;
}

bool load_binary_file(const string& path, AssetFile* output_file) {
    std::ifstream infile;
    infile.open(path, std::ios::binary);
    if (!infile.is_open()) {
        std::cerr << "Error when trying to load file: " << path << std::endl;
        return false;
    }

    u32 json_length = 0;
    u32 blob_length = 0;
    infile.seekg(0);
    infile.read(output_file->type, 4);
    infile.read((char*) &output_file->version, sizeof(u32));
    infile.read((char*) &json_length, sizeof(u32));
    infile.read((char*) &blob_length, sizeof(u32));
    output_file->json.resize(json_length);
    output_file->binary_blob.resize(blob_length);
    infile.read(output_file->json.data(), json_length);
    infile.read((char*) output_file->binary_blob.data(), blob_length);

    return true;
}

CompressionMode parse_compression(const string& f) {
    if (f == "LZ4") {
        return CompressionMode_Lz4;
    } else {
        return CompressionMode_None;
    }
}

}
