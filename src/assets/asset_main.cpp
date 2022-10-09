#include "asset_main.hpp"

#include <fstream>
#include <filesystem>
#include <chrono>

#include <lz4.h>
#include <tiny_gltf.h>
#include <stb_image.h>
#include <tiny_obj_loader.h>

#include "matrix.hpp"
#include "matrix_math.hpp"
#include "console.hpp"

#include "asset_loader.hpp"
#include "texture_asset.hpp"
#include "mesh_asset.hpp"
#include "material_asset.hpp"

#include "renderer/mesh.hpp"
#include "lib_ext/fmt_geometry.hpp"


namespace fs = std::filesystem;

namespace spellbook::assets {

const m44 gltf_fixup = m44(0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1);

void _unpack_gltf_buffer(tinygltf::Model& model, tinygltf::Accessor& accessor, vector<u8>& output_buffer) {
    int                   buffer_id     = accessor.bufferView;
    size_t                element_bsize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
    tinygltf::BufferView& buffer_view   = model.bufferViews[buffer_id];
    tinygltf::Buffer&     buffer_data   = (model.buffers[buffer_view.buffer]);
    u8*                   dataptr       = buffer_data.data.data() + accessor.byteOffset + buffer_view.byteOffset;
    int                   components    = tinygltf::GetNumComponentsInType(accessor.type);
    element_bsize *= components;
    size_t stride = buffer_view.byteStride;
    if (stride == 0) {
        stride = element_bsize;
    }

    output_buffer.resize(accessor.count * element_bsize);

    for (int i = 0; i < accessor.count; i++) {
        u8* dataindex = dataptr + stride * i;
        u8* targetptr = output_buffer.data() + element_bsize * i;

        memcpy(targetptr, dataindex, element_bsize);
    }
}

void _extract_gltf_vertices(tinygltf::Primitive& primitive, tinygltf::Model& model, vector<Vertex>& vertices) {
    tinygltf::Accessor& pos_accessor = model.accessors[primitive.attributes["POSITION"]];
    vertices.resize(pos_accessor.count);
    vector<u8> pos_data;
    _unpack_gltf_buffer(model, pos_accessor, pos_data);

    for (int i = 0; i < vertices.size(); i++) {
        if (pos_accessor.type == TINYGLTF_TYPE_VEC3) {
            if (pos_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                float* dtf = (float*) pos_data.data();

                vertices[i].position.x = *(dtf + (i * 3) + X);
                vertices[i].position.y = *(dtf + (i * 3) + Y);
                vertices[i].position.z = *(dtf + (i * 3) + Z);
            } else {
                assert_else(false);
            }
        } else {
            assert_else(false);
        }
    }

    tinygltf::Accessor& normal_accessor = model.accessors[primitive.attributes["NORMAL"]];
    vector<u8>          normal_data;
    _unpack_gltf_buffer(model, normal_accessor, normal_data);
    for (int i = 0; i < vertices.size(); i++) {
        if (normal_accessor.type == TINYGLTF_TYPE_VEC3) {
            if (normal_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                float* dtf = (float*) normal_data.data();

                vertices[i].normal[X] = *(dtf + (i * 3) + X);
                vertices[i].normal[Y] = *(dtf + (i * 3) + Y);
                vertices[i].normal[Z] = *(dtf + (i * 3) + Z);
            } else {
                assert_else(false);
            }
        } else {
            assert_else(false);
        }
    }

    tinygltf::Accessor& uv_accessor = model.accessors[primitive.attributes["TEXCOORD_0"]];
    vector<u8>          uv_data;
    _unpack_gltf_buffer(model, uv_accessor, uv_data);
    for (int i = 0; i < vertices.size(); i++) {
        if (uv_accessor.type == TINYGLTF_TYPE_VEC2) {
            if (uv_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                float* dtf = (float*) uv_data.data();

                // vec3f
                vertices[i].uv[0] = *(dtf + (i * 2) + 0);
                vertices[i].uv[1] = *(dtf + (i * 2) + 1);
            } else {
                assert_else(false && "Only FLOAT supported for UV coordinate in GLTF convert");
            }
        } else if (uv_accessor.type == TINYGLTF_TYPE_VEC3) {
            if (uv_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                float* dtf = (float*) uv_data.data();

                // vec3f
                vertices[i].uv[0] = *(dtf + (i * 3) + 0);
                vertices[i].uv[1] = *(dtf + (i * 3) + 1);
            } else {
                assert_else(false && "Only FLOAT supported for UV coordinate in GLTF convert");
            }
        } else {
            assert_else(false && "Unsupported type for UV coordinate in GLTF convert");
        }
    }

    if (primitive.attributes["COLOR"] != 0) {
        tinygltf::Accessor& color_accessor = model.accessors[primitive.attributes["COLOR"]];
        vector<u8>          color_data;
        _unpack_gltf_buffer(model, color_accessor, color_data);
        for (int i = 0; i < vertices.size(); i++) {
            if (color_accessor.type == TINYGLTF_TYPE_VEC3) {
                if (color_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                    float* dtf = (float*) color_data.data();

                    vertices[i].color[0] = *(dtf + (i * 3) + 0);
                    vertices[i].color[1] = *(dtf + (i * 3) + 1);
                    vertices[i].color[2] = *(dtf + (i * 3) + 2);
                } else {
                    assert_else(false);
                }
            } else {
                assert_else(false);
            }
        }
    }
}

void _extract_gltf_indices(tinygltf::Primitive& primitive, tinygltf::Model& model, vector<u32>& indices) {
    int index_accessor = primitive.indices;

    int    index_buffer   = model.accessors[index_accessor].bufferView;
    int    component_type = model.accessors[index_accessor].componentType;
    size_t indexsize      = tinygltf::GetComponentSizeInBytes(component_type);

    tinygltf::BufferView& indexview = model.bufferViews[index_buffer];
    int                   bufferidx = indexview.buffer;

    tinygltf::Buffer& buffindex = (model.buffers[bufferidx]);
    u8*               dataptr   = buffindex.data.data() + indexview.byteOffset;
    vector<u8>        unpackedIndices;
    _unpack_gltf_buffer(model, model.accessors[index_accessor], unpackedIndices);
    for (int i = 0; i < model.accessors[index_accessor].count; i++) {
        u32 index;
        switch (component_type) {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                u16* bfr = (u16*) unpackedIndices.data();
                index    = *(bfr + i);
            }
            break;
            case TINYGLTF_COMPONENT_TYPE_SHORT: {
                s16* bfr = (s16*) unpackedIndices.data();
                index    = *(bfr + i);
            }
            break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                u32* bfr = (u32*) unpackedIndices.data();
                index    = *(bfr + i);
            }
            break;
            case TINYGLTF_COMPONENT_TYPE_INT: {
                s32* bfr = (s32*) unpackedIndices.data();
                index    = *(bfr + i);
            }
            break;
            default:
                assert_else(false && "Only SHORT/USHORT supported for index in GLTF convert");
        }

        indices.push_back(index);
    }

    for (int i = 0; i < indices.size() / 3; i++) {
        // flip the triangle
        std::swap(indices[i * 3 + 1], indices[i * 3 + 2]);
    }
}

string _calculate_gltf_mesh_name(tinygltf::Model& model, int mesh_index, int primitive_index) {
    char buffer0[50];
    char buffer1[50];
    itoa(mesh_index, buffer0, 10);
    itoa(primitive_index, buffer1, 10);

    string meshname = "MESH_" + string{&buffer0[0]} + "_" + model.meshes[mesh_index].name;

    bool multiprim = model.meshes[mesh_index].primitives.size() > 1;
    if (multiprim) {
        meshname += "_PRIM_" + string{&buffer1[0]};
    }

    return meshname;
}

string _calculate_gltf_material_name(tinygltf::Model& model, int material_index) {
    char buffer[50];

    itoa(material_index, buffer, 10);
    string matname = "MAT_" + string{&buffer[0]} + "_" + model.materials[material_index].name;
    return matname;
}

bool _convert_image(const fs::path& input, const fs::path& relative_output_folder, const fs::path& resource_folder) {
    fs::path output = relative_output_folder / input.stem();
    output.replace_extension(".tx");

    if (fs::exists(input) && fs::exists(resource_folder / output)) {
        fs::file_time_type asset_write = fs::last_write_time(input);
        fs::file_time_type tx_write    = fs::last_write_time(resource_folder / output);
        if (tx_write > asset_write) // we already have this
            return true;
    }

    v2i  dimensions;
    int  channels;
    auto pngstart = std::chrono::high_resolution_clock::now();

    if (input.extension().string() == ".hdr") {
        f32* pixel_data = stbi_loadf((char*) input.u8string().c_str(), &dimensions.x, &dimensions.y, &channels, STBI_rgb_alpha);
        assert_else(pixel_data)
            return false;
        u64  element_count = dimensions.x * dimensions.y * 4; // we ignore channels because we force stb to make alpha
        auto pngend        = std::chrono::high_resolution_clock::now();
        auto diff          = pngend - pngstart;

        console(
        {.str = fmt_(
             "{} took {}ms to load",
             input.string(),
             std::chrono::duration_cast<std::chrono::nanoseconds>(diff).count() / 1000000.0),
         .group = "asset.import"});

        return _convert_image(input, std::span<f32>{pixel_data, (size_t) element_count}, dimensions, output, resource_folder);
        stbi_image_free(pixel_data);
    } else {
        stbi_uc* pixel_data = stbi_load((char*) input.u8string().c_str(), &dimensions.x, &dimensions.y, &channels, STBI_rgb_alpha);
        assert_else(pixel_data)
            return false;
        u64  element_count = dimensions.x * dimensions.y * 4;
        auto pngend        = std::chrono::high_resolution_clock::now();
        auto diff          = pngend - pngstart;

        console(
        {.str = fmt_(
             "{} took {}ms to load",
             input.string(),
             std::chrono::duration_cast<std::chrono::nanoseconds>(diff).count() / 1000000.0),
         .group = "asset.import"});

        return _convert_image(input, std::span<u8>{pixel_data, (size_t) element_count}, dimensions, output, resource_folder);
        stbi_image_free(pixel_data);
    }
}

bool _convert_image(
    const fs::path& source_file, std::span<u8> pixel_data, v2i dims, const fs::path& relative_output, const fs::path& resource_folder) {
    if (fs::exists(source_file) && fs::exists(resource_folder / relative_output)) {
        fs::file_time_type asset_write = fs::last_write_time(source_file);
        fs::file_time_type tx_write    = fs::last_write_time(resource_folder / relative_output);
        if (tx_write > asset_write) // we already have this
            return true;
    }
    console({.str = fmt_("_convert_image with {} pixel_data elements, and {} for dims", pixel_data.size(), dims)});

    TextureInfo texinfo;
    texinfo.original_file      = source_file.string();
    texinfo.texture_format     = (u32) vuk::Format::eR8G8B8A8Unorm;
    texinfo.dimensions         = dims;
    texinfo.original_byte_size = pixel_data.size() * sizeof(u8);

    auto              start     = std::chrono::high_resolution_clock::now();
    assets::AssetFile new_image = assets::pack_texture(&texinfo, pixel_data.data());
    auto              end       = std::chrono::high_resolution_clock::now();
    auto              diff      = end - start;
    console({.str = fmt_("{} took {}ms to compress",
                 source_file.string(),
                 std::chrono::duration_cast<std::chrono::nanoseconds>(diff).count() / 1000000.0),
             .group = "asset.import"});

    assets::save_binary_file((resource_folder / relative_output).string(), new_image);
    return true;
}

// hdr variant
bool _convert_image(
    const fs::path& source_file, std::span<f32> pixel_data, v2i dims, const fs::path& relative_output, const fs::path& resource_folder) {
    console({.str = fmt_("_convert_image with {} pixel_data elements, and {} for dims", pixel_data.size(), dims)});

    TextureInfo texinfo;
    texinfo.original_file      = source_file.string();
    texinfo.texture_format     = (u32) vuk::Format::eR32G32B32A32Sfloat;
    texinfo.dimensions         = dims;
    texinfo.original_byte_size = pixel_data.size() * sizeof(f32);

    auto              start     = std::chrono::high_resolution_clock::now();
    assets::AssetFile new_image = assets::pack_texture(&texinfo, pixel_data.data());
    auto              end       = std::chrono::high_resolution_clock::now();
    auto              diff      = end - start;
    console({.str = fmt_("{} took {}ms to compress",
                 source_file.string(),
                 std::chrono::duration_cast<std::chrono::nanoseconds>(diff).count() / 1000000.0),
             .group = "asset.import"});

    assets::save_binary_file((resource_folder / relative_output).string(), new_image);
    return true;
}

bool _convert_gltf_meshes(tinygltf::Model& model, const fs::path& input, const fs::path& output_folder, const fs::path& resource_folder) {
    tinygltf::Model* glmod = &model;
    for (auto meshindex = 0; meshindex < model.meshes.size(); meshindex++) {
        auto& glmesh = model.meshes[meshindex];

        vector<Vertex> _vertices;
        vector<u32>    _indices;

        for (auto primindex = 0; primindex < glmesh.primitives.size(); primindex++) {
            _vertices.clear();
            _indices.clear();

            string meshname = _calculate_gltf_mesh_name(model, meshindex, primindex);

            auto& primitive = glmesh.primitives[primindex];

            _extract_gltf_indices(primitive, model, _indices);
            _extract_gltf_vertices(primitive, model, _vertices);

            MeshInfo meshinfo;
            meshinfo.vertices_bsize = _vertices.size() * sizeof(Vertex);
            meshinfo.indices_bsize  = _indices.size() * sizeof(u32);
            meshinfo.index_bsize    = sizeof(u32);
            meshinfo.original_file  = input.string();

            meshinfo.bounds = assets::calculate_bounds(_vertices.data(), _vertices.size());

            assets::AssetFile newFile = assets::pack_mesh(&meshinfo, (u8*) _vertices.data(), (u8*) _indices.data());

            fs::path meshpath = output_folder / (meshname + ".mesh");

            // save to disk
            assets::save_binary_file(meshpath.string(), newFile);
        }
    }
    return true;
}

bool _convert_gltf_materials(
    tinygltf::Model& model, const fs::path& input, const fs::path& output_folder, const fs::path& resource_folder) {
    int nm = 0;
    for (auto& glmat : model.materials) {
        string matname = _calculate_gltf_material_name(model, nm);

        nm++;
        auto& pbr = glmat.pbrMetallicRoughness;

        assets::MaterialInfo new_material;
        new_material.base_effect = "defaultPBR";

        new_material.textures["baseColor"]         = "textures/white.tx";
        new_material.textures["metallicRoughness"] = "textures/white.tx";
        new_material.textures["normals"]           = "textures/white.tx";
        new_material.textures["emissive"]          = "textures/white.tx";

        int texture_indices[] = {
            pbr.baseColorTexture.index, pbr.metallicRoughnessTexture.index, glmat.normalTexture.index, glmat.emissiveTexture.index};
        string texture_names[] = {"baseColor", "metallicRoughness", "normals", "emissive"};

        for (int i = 0; i < 4; i++) {
            int texture_index = texture_indices[i];
            if (texture_index < 0)
                continue;
            auto image     = model.textures[texture_index];
            auto baseImage = model.images[image.source];

            if (baseImage.name == "")
                baseImage.name = texture_names[i];

            fs::path baseColorPath = _convert_to_relative(output_folder / baseImage.name, resource_folder);
            baseColorPath.replace_extension(".tx");
            _convert_image(input, baseImage.image, v2i{baseImage.width, baseImage.height}, baseColorPath, resource_folder);
            new_material.textures[texture_names[i]] = baseColorPath.string();
        }

        new_material.properties["base_color_r"] = (f32) pbr.baseColorFactor[0];
        new_material.properties["base_color_g"] = (f32) pbr.baseColorFactor[1];
        new_material.properties["base_color_b"] = (f32) pbr.baseColorFactor[2];
        new_material.properties["base_color_a"] = (f32) pbr.baseColorFactor[3];
        new_material.properties["roughness"]    = pbr.roughnessFactor;
        new_material.properties["metallic"]     = pbr.metallicFactor;
        new_material.properties["normal"]       = new_material.textures["normals"] == "textures/white.tx" ? 0.0f : 0.5f;
        // cel shading should have minimal normals
        new_material.properties["emissive"] = new_material.textures["emissive"] == "textures/white.tx" ? 0.0f : 1.0f;
        fs::path materialPath               = output_folder / (matname + ".mat");

        if (glmat.alphaMode.compare("BLEND") == 0) {
            new_material.transparency = TransparencyMode_Transparent;
        } else {
            new_material.transparency = TransparencyMode_Opaque;
        }

        assets::AssetFile newFile = assets::pack_material(&new_material);

        // save to disk
        assets::save_binary_file(materialPath.string(), newFile);
    }
    return true;
}

bool _convert_gltf_nodes(tinygltf::Model& model, const fs::path& input, const fs::path& output_folder, const fs::path& resource_folder) {
    assets::PrefabInfo prefab;

    // calculate parent hierarchies
    // gltf stores children, but we want parent
    for (int i = 0; i < model.nodes.size(); i++) {
        for (auto c : model.nodes[i].children) {
            prefab.node_parents[c] = i;
        }
    }

    vector<u64> multimat_nodes;
    for (int i = 0; i < model.nodes.size(); i++) {
        auto& node = model.nodes[i];

        if (node.mesh >= 0 && model.meshes[node.mesh].primitives.size() == 1) {
            auto mesh = model.meshes[node.mesh];

            auto primitive = mesh.primitives[0];
            int  material  = primitive.material;
            assert_else(material >= 0 && "Need material on node, default mat NYI");
            string meshname = _calculate_gltf_mesh_name(model, node.mesh, 0);
            string matname  = _calculate_gltf_material_name(model, material);

            fs::path meshpath     = _convert_to_relative(output_folder / (meshname + ".mesh"), resource_folder);
            fs::path materialpath = _convert_to_relative(output_folder / (matname + ".mat"), resource_folder);

            assets::PrefabNodeInfo nmesh;
            nmesh.node_name     = model.nodes[i].name;
            nmesh.mesh_path     = meshpath.string();
            nmesh.material_path = materialpath.string();
            nmesh.matrix        = calculate_matrix(node);

            prefab.node_infos[i] = nmesh;
        }
        // Serving hierarchy
        else {
            assets::PrefabNodeInfo nmesh;
            nmesh.node_name      = node.name;
            nmesh.mesh_path      = "";
            nmesh.material_path  = "";
            nmesh.matrix         = calculate_matrix(node);
            prefab.node_infos[i] = nmesh;

            if (node.mesh >= 0)
                multimat_nodes.push_back(i);
        }
    }

    for (int i = 0; i < model.nodes.size(); i++) {
        auto it = prefab.node_parents.find(i);
        if (it == prefab.node_parents.end()) {
            prefab.node_infos[i].matrix = gltf_fixup * prefab.node_infos[i].matrix;
        }
    }

    int nodeindex = model.nodes.size();
    // iterate nodes with multiple materials, convert each submesh into a node
    for (int i = 0; i < multimat_nodes.size(); i++) {
        auto  multimat_node_index = multimat_nodes[i];
        auto& multimat_node       = model.nodes[multimat_node_index];

        if (multimat_node.mesh < 0)
            break;

        auto mesh = model.meshes[multimat_node.mesh];

        for (int primindex = 0; primindex < mesh.primitives.size(); primindex++) {
            auto primitive = mesh.primitives[primindex];
            int  new_node  = nodeindex++;

            char buffer[50];

            itoa(primindex, buffer, 10);

            int    material = primitive.material;
            auto   mat      = model.materials[material];
            string matname  = _calculate_gltf_material_name(model, material);
            string meshname = _calculate_gltf_mesh_name(model, multimat_node.mesh, primindex);

            fs::path materialpath = _convert_to_relative(output_folder / (matname + ".mat"), resource_folder);
            fs::path meshpath     = _convert_to_relative(output_folder / (meshname + ".mesh"), resource_folder);

            assets::PrefabNodeInfo new_mesh;
            new_mesh.node_name     = prefab.node_infos[i].node_name + "_PRIM_" + &buffer[0];
            new_mesh.mesh_path     = meshpath.string();
            new_mesh.material_path = materialpath.string();
            new_mesh.matrix        = m44::identity();

            prefab.node_infos[new_node]   = new_mesh;
            prefab.node_parents[new_node] = multimat_node_index;
        }
    }

    assets::AssetFile new_file   = assets::pack_prefab(&prefab);
    fs::path          scene_path = output_folder / input.stem();
    scene_path.replace_extension(".pfb");
    // save to disk
    assets::save_binary_file(scene_path.string(), new_file);
    return true;
}

Prefab convert_to_prefab(const fs::path&, const fs::path& output_folder, const fs::path& resource_folder) {

}

m44 calculate_matrix(tinygltf::Node& node) {
    m44 matrix;

    // node has a matrix
    if (node.matrix.size() > 0) {
        for (int n = 0; n < 16; n++) {
            matrix[n] = node.matrix[n];
        }
    }
    // separate transform
    else {
        m44 translation = m44::identity();
        if (node.translation.size() > 0) {
            translation = m::translate(v3{(f32) node.translation[0], (f32) node.translation[1], (f32) node.translation[2]});
        }

        m44 rotation = m44::identity();
        if (node.rotation.size() > 0) {
            quat rot((f32) node.rotation[0], (f32) node.rotation[1], (f32) node.rotation[2], (f32) node.rotation[3]);
            rotation = m::quat2rotation44(rot);
        }

        m44 scale = m44::identity();
        if (node.scale.size() > 0) {
            scale = m::scale(v3{(f32) node.scale[0], (f32) node.scale[1], (f32) node.scale[2]});
        }
        matrix = (translation * rotation * scale); // * gltf_fixup;
    }

    return matrix;
};

bool convert(const fs::path& input, const fs::path& resource_folder, const string& subfolder) {
    fs::create_directory(resource_folder);

    const auto& ext = input.extension().string();

    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".psd" || ext == ".hdr" || ext == ".bmp" || ext == ".tga" ||
        ext == ".gif") {
        auto folder = resource_folder / fs::path("textures");
        fs::create_directory(folder);
        return _convert_image(input, fs::path("textures"), resource_folder);
    } else if (ext == ".gltf" || ext == ".glb") {
        // TODO: gltf can't support null materials
        tinygltf::Model    model;
        tinygltf::TinyGLTF loader;
        string             err, warn;

        bool ret = ext == ".gltf"
                       ? loader.LoadASCIIFromFile(&model, &err, &warn, input.string())
                       : loader.LoadBinaryFromFile(&model, &err, &warn, input.string());

        if (!warn.empty())
            console_error(fmt_("Conversion warning while loading \"{}\": {}", input.string(), warn), "asset.import", ErrorType_Warning);
        if (!err.empty())
            console_error(fmt_("Conversion error while loading \"{}\": {}", input.string(), err), "asset.import", ErrorType_Severe);
        if (!ret)
            return false;

        auto folder = resource_folder / fs::path(input.stem().string() + "_GLTF");
        fs::create_directory(folder);

        _convert_gltf_meshes(model, input, folder, resource_folder);
        _convert_gltf_materials(model, input, folder, resource_folder);
        _convert_gltf_nodes(model, input, folder, resource_folder);
    } else {
        console_error(fmt_("Unsupported file type for conversion for file \"{}\"", input.string()), "asset.import", ErrorType_Warning);
        return false;
    }
    return true;
}

fs::path _convert_to_relative(const fs::path& path, const fs::path& resource_path) {
    return path.lexically_proximate(resource_path);
}

}
