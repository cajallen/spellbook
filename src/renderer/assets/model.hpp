#pragma once

#include "general/string.hpp"
#include "general/vector.hpp"
#include "general/umap.hpp"
#include "general/memory.hpp"
#include "general/id_ptr.hpp"
#include "general/math/matrix.hpp"
#include "general/file/file_path.hpp"
#include "renderer/assets/skeleton.hpp"

namespace fs = std::filesystem;

namespace spellbook {

struct MeshCPU;
struct MaterialCPU;
struct StaticRenderable;
struct Renderable;
struct RenderScene;
struct SkeletonCPU;
struct SkeletonGPU;

struct ModelExternal {
    static constexpr string_view extension() { return "?"; }
    static constexpr string_view dnd_key() { return "DND_MODEL_EXTERNAL"; }
    static FilePath folder() { return get_external_resource_folder(); }
    static std::function<bool(const FilePath&)> path_filter() { return [](const FilePath& path) { return vector<string>{".glb", ".gltf"}.contains(path.extension()); }; }
};

struct ModelCPU : Resource {
    struct Node {
        string name;
        FilePath mesh_asset_path;
        FilePath material_asset_path;
        m44 transform = {};
        
        id_ptr<Node> parent = {};
        vector<id_ptr<Node>> children = {};
        
        m44 cached_transform;
        
        void cache_transform();
    };
    
    vector<id_ptr<Node>> nodes = {};
    id_ptr<Node> root_node = id_ptr<Node>::null();
    unique_ptr<SkeletonCPU> skeleton;

    vector<ModelCPU> split();

    ModelCPU() = default;
    ModelCPU(ModelCPU&&) = default;
    ModelCPU(const ModelCPU& oth);

    ModelCPU& operator = (const ModelCPU& oth);

    static constexpr string_view extension() { return ".sbjmod"; }
    static constexpr string_view dnd_key() { return "DND_MODEL"; }
    static FilePath folder() { return get_resource_folder() + "models"; }
    static std::function<bool(const FilePath&)> path_filter() { return [](const FilePath& path) { return path.extension() == ModelCPU::extension(); }; }
};

JSON_IMPL(ModelCPU::Node, name, mesh_asset_path, material_asset_path, transform, parent, children);

struct ModelGPU {
    umap<ModelCPU::Node*, Renderable*> renderables;
    unique_ptr<SkeletonGPU> skeleton;

    ModelGPU() {
        renderables = {};
        skeleton = {};
    }
    ModelGPU(const ModelGPU&) = delete;
    ModelGPU(ModelGPU&& other) = default;
    ModelGPU& operator=(ModelGPU&&) = default;
};

template <>
bool     save_resource(const ModelCPU& resource_file);
template <>
ModelCPU& load_resource(const FilePath& input_path, bool assert_exist, bool clear_cache);

ModelGPU instance_model(RenderScene&, const ModelCPU&, bool frame = false);
vector<StaticRenderable*> instance_static_model(RenderScene&, const ModelCPU&);
void     deinstance_model(RenderScene&, const ModelGPU&);
void     deinstance_static_model(RenderScene&, const vector<StaticRenderable*>&);
ModelCPU convert_to_model(const FilePath& input_path, const FilePath& output_folder, const string& output_name, bool y_up = true, bool replace_existing_poses = false);

bool inspect(ModelCPU* model, RenderScene* render_scene = nullptr);

}
