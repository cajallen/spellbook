#pragma once

#include "general/umap.hpp"
#include "general/memory.hpp"
#include "general/id_ptr.hpp"
#include "general/math/matrix.hpp"
#include "general/file/file_path.hpp"
#include "general/file/resource.hpp"
#include "renderer/assets/skeleton.hpp"

namespace spellbook {

struct MeshCPU;
struct MaterialCPU;
struct StaticRenderable;
struct Renderable;
struct RenderScene;

struct ModelExternal {
    static constexpr string_view extension() { return "?"; }
    static constexpr string_view dnd_key() { return "DND_MODEL_EXTERNAL"; }
    static FilePath folder() { return get_external_resource_folder(); }
    static function<bool(const FilePath&)> path_filter() { return [](const FilePath& path) { return vector<string>{".glb", ".gltf"}.contains(path.extension()); }; }
};

struct ModelCPU : Resource {
    struct Node {
        string name;
        uint32 mesh_selection = 0;
        vector<FilePath> mesh_path_options;
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
    static function<bool(const FilePath&)> path_filter() { return [](const FilePath& path) { return path.extension() == ModelCPU::extension(); }; }
};

ModelCPU::Node from_jv_impl(const json_value& jv, ModelCPU::Node* _);
json_value to_jv(const ModelCPU::Node& value);


struct ModelGPU {
    umap<ModelCPU::Node*, Renderable*> renderables;
    unique_ptr<SkeletonGPU> skeleton;

    ModelGPU() : renderables(), skeleton() {}
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
