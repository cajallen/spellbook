
#include "asset_loader.hpp"

#include <fstream>
#include <iostream>

#include "console.hpp"

namespace spellbook {


void save_asset_file(const AssetFile& asset_file) {
    std::ofstream outfile;
    outfile.open(asset_file.file_name, std::ios::binary | std::ios::out);
    
    assert_else(outfile.is_open())
        return;

    string json_string = dump_json(asset_file.asset_json);
    
    const u32 version     = asset_file.version;
    const u32 json_length = (u32) json_string.size();
    const u32 blob_length = (u32) asset_file.binary_blob.size();
    outfile.write(asset_file.type.data(), 4);
    outfile.write((const char*) &version, sizeof(u32));
    outfile.write((const char*) &json_length, sizeof(u32));
    outfile.write((const char*) &blob_length, sizeof(u32));
    outfile.write(json_string.data(), json_length);
    outfile.write((const char*) asset_file.binary_blob.data(), asset_file.binary_blob.size());
    outfile.close();
}

AssetFile load_binary_file(const string& file_name) {
    std::ifstream infile;
    infile.open(file_name, std::ios::binary);
    
    assert_else(infile.is_open())
        return {};

    AssetFile asset_file;
    string json_string;
    
    u32 json_length = 0;
    u32 blob_length = 0;
    infile.seekg(0);
    infile.read(asset_file.type.data(), 4);
    infile.read((char*) &asset_file.version, sizeof(u32));
    infile.read((char*) &json_length, sizeof(u32));
    infile.read((char*) &blob_length, sizeof(u32));
    json_string.resize(json_length);
    asset_file.binary_blob.resize(blob_length);
    infile.read(json_string.data(), json_length);
    infile.read((char*) asset_file.binary_blob.data(), blob_length);

    asset_file.asset_json = parse(json_string);

    return asset_file;
}

}
