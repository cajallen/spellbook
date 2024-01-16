#pragma once

#include "general/umap.hpp"
#include "general/string.hpp"
#include "general/file/file_path.hpp"

#include "assets/mesh.hpp"
#include "assets/material.hpp"
#include "assets/texture.hpp"

namespace spellbook {

struct GPUAssetCache {
    umap<uint64, MeshGPU>     meshes;
    umap<uint64, MaterialGPU> materials;
    umap<uint64, TextureGPU>  textures;
    umap<uint64, FilePath>    paths;

    void upload_defaults();
    MeshGPU* get_mesh(uint64 id);
    MaterialGPU* get_material(uint64 id);
    TextureGPU* get_texture(uint64 id);
    MeshGPU& get_mesh_or_upload(uint64 id);
    MaterialGPU& get_material_or_upload(uint64 id);
    TextureGPU& get_texture_or_upload(const FilePath& asset_path);

    void clear_frame_allocated_assets();
    void clear();
};

inline GPUAssetCache& get_gpu_asset_cache() {
    static GPUAssetCache gpu_asset_cache;
    return gpu_asset_cache;
}

}