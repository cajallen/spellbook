#include "prefab.hpp"

#include <filesystem>

#include "tiny_gltf.h"

#include "game.hpp"
#include "matrix_math.hpp"
#include "mesh_asset.hpp"

#include "renderer/renderer.hpp"
#include "renderer/render_scene.hpp"

namespace fs = std::filesystem;

namespace spellbook {

vector<PrefabCPU> PrefabCPU::split() {
    auto traverse = [](PrefabCPU& prefab, id_ptr<PrefabCPU::Node> node, auto&& traverse) -> void {
        for (id_ptr<Node> child : node->children) {
            prefab.nodes.insert_back(child);
            traverse(prefab, child, traverse);
        }
    };

    vector<PrefabCPU> prefabs;
    prefabs.resize(root_node->children.size());
    u32 i = 0;
    for (id_ptr<Node> child : root_node->children) {
        child->parent = id_ptr<Node>::null(); 
        traverse(prefabs[i++], child, traverse);
    }

    return prefabs;
}

void save_prefab(const PrefabCPU& prefab) {
    json j;
    j["root_node"] = make_shared<json_value>(prefab.root_node);

    vector<json_value> json_nodes;
    for (id_ptr<PrefabCPU::Node> node : prefab.nodes) {
        json json_node;
        json_node["node"] = make_shared<json_value>(*node);
        json_node["id"] = make_shared<json_value>(node);
        json_nodes.insert_back((json_value) json_node);
    }
    j["nodes"] = make_shared<json_value>(json_nodes);
    file_dump(j, prefab.file_name);
}

PrefabCPU load_prefab(const string_view file_name) {
    PrefabCPU prefab;
    json j = parse_file(file_name);
    if (j.contains("root_node"))
        prefab.root_node = id_ptr<PrefabCPU::Node>(*j["root_node"]);

    if (j.contains("nodes")) {
        for (const json_value& jv : j["nodes"]->get_list()) {
            auto json_node = json(jv);
            json_node["node"];
            json_node["id"];
        }
    }
    return prefab;
}

PrefabGPU instance_prefab(RenderScene& render_scene, const PrefabCPU& prefab) {
    PrefabGPU prefab_gpu;

    for (id_ptr<PrefabCPU::Node> node_ptr : prefab.nodes) {
        const PrefabCPU::Node& node = *node_ptr;
        Renderable renderable = Renderable(game.renderer.find_mesh(node.name), game.renderer.find_material(node.material), node.transform);
        auto new_renderable = render_scene.add_renderable(renderable);
        prefab_gpu.renderables.insert_back(new_renderable);
    }
    
    return prefab_gpu;
}

const m44 gltf_fixup = m44(0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1);

fs::path _convert_to_relative(const fs::path& path, const fs::path& resource_path) {
    return path.lexically_proximate(resource_path);
}

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
                auto bfr = (u16*) unpackedIndices.data();
                index    = *(bfr + i);
            }
            break;
            case TINYGLTF_COMPONENT_TYPE_SHORT: {
                auto bfr = (s16*) unpackedIndices.data();
                index    = *(bfr + i);
            }
            break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                auto bfr = (u32*) unpackedIndices.data();
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

        indices.insert_back(index);
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

bool _convert_gltf_meshes(tinygltf::Model& model, const fs::path& input, const fs::path& output_folder, const fs::path& resource_folder) {
    tinygltf::Model* glmod = &model;
    for (auto meshindex = 0; meshindex < model.meshes.size(); meshindex++) {
        auto& glmesh = model.meshes[meshindex];

        vector<Vertex> vertices;
        vector<u32>    indices;

        for (auto primindex = 0; primindex < glmesh.primitives.size(); primindex++) {
            vertices.clear();
            indices.clear();

            string meshname = _calculate_gltf_mesh_name(model, meshindex, primindex);

            auto& primitive = glmesh.primitives[primindex];

            _extract_gltf_indices(primitive, model, indices);
            _extract_gltf_vertices(primitive, model, vertices);

            MeshInfo mesh_info;
            mesh_info.vertices_bsize = vertices.size() * sizeof(Vertex);
            mesh_info.indices_bsize  = indices.size() * sizeof(u32);
            mesh_info.index_bsize    = sizeof(u32);

            MeshCPU mesh_cpu;
            
            save_mesh(mesh_cpu, mesh_info, (u8*) vertices.data(), (u8*) indices.data());

            fs::path meshpath = output_folder / (meshname + ".mesh");

            // save to disk
            save_binary_file(meshpath.string(), newFile);
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

m44 _calculate_matrix(tinygltf::Node& node) {
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
            translation = math::translate(v3{(f32) node.translation[0], (f32) node.translation[1], (f32) node.translation[2]});
        }

        m44 rotation = m44::identity();
        if (node.rotation.size() > 0) {
            quat rot((f32) node.rotation[0], (f32) node.rotation[1], (f32) node.rotation[2], (f32) node.rotation[3]);
            rotation = math::quat2rotation44(rot);
        }

        m44 scale = m44::identity();
        if (node.scale.size() > 0) {
            scale = math::scale(v3{(f32) node.scale[0], (f32) node.scale[1], (f32) node.scale[2]});
        }
        matrix = (translation * rotation * scale); // * gltf_fixup;
    }

    return matrix;
};

PrefabCPU convert_to_prefab(const string_view file_name) {
    
}


}