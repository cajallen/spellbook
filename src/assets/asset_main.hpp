#include <filesystem>
#include <tinygltf/tiny_gltf.h>

#include "vector.hpp"
#include "string.hpp"
#include "geometry.hpp"

#include "renderer/vertex.hpp"
#include "renderer/prefab.hpp"
#include "renderer/texture.hpp"

namespace fs = std::filesystem;

namespace spellbook::assets {

fs::path _convert_to_relative(const fs::path& path, const fs::path& resource_folder);
string _calculate_gltf_material_name(tinygltf::Model& model, int material_index);
string _calculate_gltf_mesh_name(tinygltf::Model& model, int mesh_index, int primitive_index);
void _unpack_gltf_buffer(tinygltf::Model& model, tinygltf::Accessor& accessor, vector<u8>& output_buffer);
void _extract_gltf_vertices(tinygltf::Primitive& primitive, tinygltf::Model& model, vector<Vertex>& vertices);
void _extract_gltf_indices(tinygltf::Primitive& primitive, tinygltf::Model& model, vector<u32>& indices);
bool _convert_gltf_meshes(tinygltf::Model& model, const fs::path& input, const fs::path& output_folder, const fs::path& resource_folder);
bool _convert_gltf_materials(tinygltf::Model& model, const fs::path& input, const fs::path& output_folder, const fs::path& resource_folder);
bool _convert_gltf_nodes(tinygltf::Model& model, const fs::path& input, const fs::path& output_folder, const fs::path& resource_folder);

TextureCPU convert_to_texture(const fs::path& source, span<f32> pixels, v2i dims, const fs::path& relative_output, const fs::path& resource_folder);
TextureCPU convert_to_texture(const fs::path& source, span<u8> pixels, v2i dims, const fs::path& relative_output, const fs::path& resource_folder);
TextureCPU convert_to_texture(const fs::path& input, const fs::path& relative_output_folder, const fs::path& resource_folder);
PrefabCPU convert_to_prefab(const fs::path&, const fs::path& output_folder, const fs::path& resource_folder);

bool convert(const fs::path& input, const fs::path& resource_folder, const string& subfolder = "");

}
