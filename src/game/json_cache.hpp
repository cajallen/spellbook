#pragma once

#include "json.hpp"
#include "renderer/assets/asset_loader.hpp"

namespace spellbook {

// TODO: purge if file is outdated

struct AssetCache {
    umap<string, json> parsed_jsons;
    umap<string, AssetFile> parsed_assets;

    json& load_json(const string& file_path);
    AssetFile& load_asset(const string& file_path);

    vector<string> load_dependencies(json& j);
};

}
