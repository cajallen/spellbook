#include "loader.hpp"

#include "renderer.hpp"
#include "console.hpp"
#include "assets/texture_asset.hpp"
#include "assets/prefab_asset.hpp"
#include "assets/material_asset.hpp"
#include "assets/mesh_asset.hpp"
#include "matrix_math.hpp"

#include "render_scene.hpp"
#include "renderable.hpp"

#include "lib_ext/fmt_geometry.hpp"
#include "lib_ext/fmt_renderer.hpp"

#include "vuk/Partials.hpp"


const m44 gltf_fixup = m44(0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1);

bool load_mesh(Renderer& renderer, const string& name, const string& asset_path) {
    assets::AssetFile file;
    bool              loaded = assets::load_binary_file(asset_path, &file);
    assert_else(loaded) return false;
    assets::MeshInfo meshinfo = assets::read_mesh_info(&file);

    vector<u8> vertex_buffer;
    vector<u8> index_buffer;
    vertex_buffer.resize(meshinfo.vertices_bsize);
    index_buffer.resize(meshinfo.indices_bsize);

    assets::unpack_mesh(&meshinfo, file.binary_blob.data(), file.binary_blob.size(), vertex_buffer.data(), index_buffer.data());

    MeshCPU mesh_cpu;
    mesh_cpu.name           = name;
    mesh_cpu.bounds.extents = meshinfo.bounds.extents;
    mesh_cpu.bounds.origin  = meshinfo.bounds.origin;
    mesh_cpu.bounds.radius  = meshinfo.bounds.radius;
    mesh_cpu.bounds.valid   = true;

    mesh_cpu.indices.resize(index_buffer.size() / sizeof(u32));
    for (int i = 0; i < mesh_cpu.indices.size(); i++) {
        u32* unpacked_indices = (u32*) index_buffer.data();
        mesh_cpu.indices[i]   = unpacked_indices[i];
    }

    Vertex* file_vertices = (Vertex*) vertex_buffer.data();

    mesh_cpu.vertices.resize(vertex_buffer.size() / sizeof(Vertex));

    for (int i = 0; i < mesh_cpu.vertices.size(); i++) {
        mesh_cpu.vertices[i] = file_vertices[i];
    }

    for (int i = 0; i + 2 < mesh_cpu.indices.size(); i += 3) {
        auto index1                       = mesh_cpu.indices[i + 0];
        auto index2                       = mesh_cpu.indices[i + 1];
        auto index3                       = mesh_cpu.indices[i + 2];
        auto tangent                      = get_tangent(mesh_cpu.vertices[index1], mesh_cpu.vertices[index2], mesh_cpu.vertices[index3]);
        mesh_cpu.vertices[index1].tangent = tangent;
        mesh_cpu.vertices[index2].tangent = tangent;
        mesh_cpu.vertices[index3].tangent = tangent;
    }
    renderer.upload_mesh(mesh_cpu, false);
    console({.str = fmt_("Loaded mesh from \"{}\" with {}v, {}t", asset_path, mesh_cpu.vertices.size(), mesh_cpu.indices.size() / 3),
        .group    = "asset.load"});

    return true;
}
bool load_material(Renderer& renderer, const string& name, const string& asset_path) {
    assets::AssetFile file;
    bool              loaded = assets::load_binary_file(asset_path, &file);
    assert_else(loaded) return false;
    assets::MaterialInfo info = read_material_info(&file);

    // load .tx files, PBR keys are assumed for matCPU
    for (auto& [key, value] : info.textures) {
        load_texture(renderer, name + "_" + key, renderer.resource_path(value));
    }
    MaterialCPU mat_cpu;
    mat_cpu.name                       = name;
    mat_cpu.base_color_texture         = name + "_" + "baseColor";
    mat_cpu.metallic_roughness_texture = name + "_" + "metallicRoughness";
    mat_cpu.normal_texture             = name + "_" + "normals";
    mat_cpu.emissive_texture           = name + "_" + "emissive";
    mat_cpu.base_color_tint            = Color {
        info.properties["base_color_r"], info.properties["base_color_g"], info.properties["base_color_b"], info.properties["base_color_a"]};
    mat_cpu.roughness_factor = info.properties["roughness"];
    mat_cpu.metallic_factor  = info.properties["metallic"];
    mat_cpu.normal_factor    = info.properties["normal"];
    mat_cpu.emissive_tint.r  = info.properties["emissive_r"];
    mat_cpu.emissive_tint.g  = info.properties["emissive_g"];
    mat_cpu.emissive_tint.b  = info.properties["emissive_b"];
    mat_cpu.emissive_tint.a  = info.properties["emissive_a"];
    renderer.upload_material(mat_cpu, false);

    return true;
}
