#pragma once

#include "general/json.hpp"
#include "renderer/assets/asset_loader.hpp"
#include "general/file_path.hpp"

namespace spellbook {

// TODO: purge if file is outdated

struct DiskCache {
    umap<FilePath, json> parsed_jsons;
    umap<FilePath, AssetFile> parsed_assets;

    json& load_json(const FilePath& file_path);
    AssetFile& load_asset(const FilePath& file_path);

    vector<FilePath> load_dependencies(json& j);
};

}
