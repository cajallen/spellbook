#pragma once

#include "umap.hpp"
#include "json.hpp"
#include "matrix.hpp"
#include "asset_loader.hpp"

#include <string>
using std::string;

namespace assets {

struct PrefabNodeInfo {
    string node_name     = "";
    string material_path = "";
    string mesh_path     = "";
    m44    matrix        = m44::identity();

    PrefabNodeInfo() = default;
    JSON_IMPL(PrefabNodeInfo, node_name, material_path, mesh_path, matrix);
};
struct PrefabInfo {

    // points to matrix array in the blob
    umap<u64, PrefabNodeInfo> node_infos   = {};
    umap<u64, u64>            node_parents = {};

    PrefabInfo() = default;
    JSON_IMPL(PrefabInfo, node_infos, node_parents);
};

PrefabInfo read_prefab_info(AssetFile* file);
AssetFile  pack_prefab(PrefabInfo* info);
} // namespace assets