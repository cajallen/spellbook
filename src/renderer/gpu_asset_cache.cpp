#include "gpu_asset_cache.hpp"

#include "general/logger.hpp"

#include "renderer/draw_functions.hpp"
#include "renderer/assets/material.hpp"

namespace spellbook {

MeshGPU* GPUAssetCache::get_mesh(uint64 id) {
    if (meshes.contains(id))
        return &meshes[id];
    return nullptr;
}

MaterialGPU* GPUAssetCache::get_material(uint64 id) {
    if (materials.contains(id))
        return &materials.at(id);
    return nullptr;
}

TextureGPU* GPUAssetCache::get_texture(uint64 id) {
    if (textures.contains(id))
        return &textures[id];
    return nullptr;
}

MeshGPU& GPUAssetCache::get_mesh_or_upload(uint64 id) {
    if (meshes.contains(id))
        return meshes[id];
    assert_else(paths.contains(id));
    upload_mesh(load_mesh(paths[id]));
    return meshes[id];
}

MaterialGPU& GPUAssetCache::get_material_or_upload(uint64 id) {
    if (materials.contains(id))
        return materials.at(id);
    assert_else(paths.contains(id));
    upload_material(load_resource<MaterialCPU>(paths.at(id)));
    return materials.at(id);
}

TextureGPU& GPUAssetCache::get_texture_or_upload(const FilePath& asset_path) {
    assert_else(asset_path.is_file());
    uint64 hash = hash_path(asset_path);
    if (textures.contains(hash))
        return textures[hash];
    upload_texture(load_texture(asset_path));
    return textures[hash];
}


void GPUAssetCache::upload_defaults() {
    TextureCPU tex_white_upload {
        .size = v2i(8, 8),
        .format = vuk::Format::eR8G8B8A8Srgb,
        .pixels = vector<uint8>(8 * 8 * 4, 255)
    };
    tex_white_upload.file_path = "white"_symbolic;
    upload_texture(tex_white_upload);

    constexpr uint32 grid_size = 1024;
    TextureCPU tex_grid_upload {
        .size = v2i(grid_size, grid_size),
        .format = vuk::Format::eR8G8B8A8Srgb,
        .pixels = vector<uint8>(grid_size * grid_size * 4, 255)
    };
    tex_grid_upload.file_path = "grid"_symbolic;

    // do border
    for (uint32 i = 0; i < (grid_size - 1); i++) {
        for (uint32 pixel_pos : vector<uint32>{i, i * grid_size + (grid_size - 1), (grid_size - 1) * grid_size + i + 1, i * grid_size + grid_size}) {
            tex_grid_upload.pixels[pixel_pos * 4 + 0] = 0;
            tex_grid_upload.pixels[pixel_pos * 4 + 1] = 0;
            tex_grid_upload.pixels[pixel_pos * 4 + 2] = 0;
        }
    }
    upload_texture(tex_grid_upload);

    TextureCPU default_tex = {
        .size = {2, 2},
        .format = vuk::Format::eR8G8B8A8Srgb,
        .pixels = {255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255}
    };
    default_tex.file_path = "default"_symbolic;
    upload_texture(default_tex);

    MaterialCPU default_mat = {
        .color_tint = palette::black,
    };
    default_mat.file_path = "default"_symbolic;
    upload_material(default_mat);


    MeshCPU default_mesh   = generate_cube(v3(0), v3(1));
    default_mesh.file_path = "default"_symbolic;
    upload_mesh(default_mesh);
}

void GPUAssetCache::clear_frame_allocated_assets() {
    auto mesh_it = meshes.begin();
    while (mesh_it != meshes.end()) {
        if (mesh_it->second.frame_allocated)
            mesh_it = meshes.erase(mesh_it);
        else
            mesh_it++;
    }
    auto tex_it = textures.begin();
    while (tex_it != textures.end()) {
        if (tex_it->second.frame_allocated)
            tex_it = textures.erase(tex_it);
        else
            tex_it++;
    }
    auto mat_it = materials.begin();

    while (mat_it != materials.end()) {
        if (mat_it->second.frame_allocated)
            mat_it = materials.erase(mat_it);
        else
            mat_it++;
    }
}


void GPUAssetCache::clear() {
    meshes.clear();
    materials.clear();
    textures.clear();
    paths.clear();
}

}