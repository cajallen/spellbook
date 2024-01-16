#include "mesh.hpp"

#include <imgui/imgui.h>
#include <vuk/Partials.hpp>
#include <lz4/lz4.h>

#include "extension/fmt.hpp"
#include "general/math/math.hpp"
#include "general/logger.hpp"
#include "renderer/renderer.hpp"
#include "renderer/gpu_asset_cache.hpp"

namespace spellbook {

static v3 get_tangent(Vertex a, Vertex b, Vertex c) {
    v3 ab    = b.position - a.position;
    v3 ac    = c.position - a.position;
    v2 ab_uv = b.uv - a.uv;
    v2 ac_uv = c.uv - a.uv;
    if (ab_uv == v2(0) || ac_uv == v2(0) || ab_uv == ac_uv) {
        ab_uv = v2(1, 0);
        ac_uv = v2(0, 1);
    }
    auto f            = 1.0f / math::cross(ab_uv, ac_uv);
    v3   unnormalized = f * (ac_uv.y * ab - ab_uv.y * ac);
    if (unnormalized == v3(0))
        unnormalized = v3(1, 0, 0);
    return math::normalize(unnormalized);
}

void MeshCPU::fix_tangents() {
    for (int i = 0; (i + 2) < indices.size(); i+=3) {
        v3 tangent = get_tangent(vertices[indices[i+0]],vertices[indices[i+1]],vertices[indices[i+2]]);
        vertices[indices[i+0]].tangent = vertices[indices[i+1]].tangent = vertices[indices[i+2]].tangent = tangent;
    }
}

uint64 upload_mesh(const MeshCPU& mesh_cpu, bool frame_allocation) {
    if (!mesh_cpu.file_path.is_file())
        return 0;
    uint64 mesh_cpu_hash = hash_path(mesh_cpu.file_path);
    if (get_gpu_asset_cache().meshes.contains(mesh_cpu_hash))
        return mesh_cpu_hash;
    MeshGPU         mesh_gpu;
    mesh_gpu.frame_allocated = frame_allocation;
    vuk::Allocator& alloc                = frame_allocation ? *get_renderer().frame_allocator : *get_renderer().global_allocator;
    auto            [vert_buf, vert_fut] = vuk::create_buffer(alloc, vuk::MemoryUsage::eGPUonly, vuk::DomainFlagBits::eTransferOnTransfer, std::span(mesh_cpu.vertices));
    mesh_gpu.vertex_buffer               = std::move(vert_buf);
    auto [idx_buf, idx_fut]              = vuk::create_buffer(alloc, vuk::MemoryUsage::eGPUonly, vuk::DomainFlagBits::eTransferOnTransfer, std::span(mesh_cpu.indices));
    mesh_gpu.index_buffer                = std::move(idx_buf);
    mesh_gpu.index_count                 = mesh_cpu.indices.size();
    mesh_gpu.vertex_count                = mesh_cpu.vertices.size();

    get_renderer().enqueue_setup(std::move(vert_fut));
    get_renderer().enqueue_setup(std::move(idx_fut));

    get_gpu_asset_cache().meshes[mesh_cpu_hash] = std::move(mesh_gpu);
    get_gpu_asset_cache().paths[mesh_cpu_hash] = mesh_cpu.file_path;
    return mesh_cpu_hash;
}

MeshCPU load_mesh(const FilePath& file_path) {
    AssetFile& asset_file = get_file_cache().load_asset(file_path);

    MeshInfo mesh_info = from_jv<MeshInfo>(*asset_file.asset_json["mesh_info"]);
    MeshCPU mesh_cpu = from_jv<MeshCPU>(*asset_file.asset_json["mesh_cpu"]);
    mesh_cpu.file_path = file_path;

    vector<uint8> decompressed;
    decompressed.resize(mesh_info.vertices_bsize + mesh_info.indices_bsize);
    LZ4_decompress_safe((const char*) asset_file.binary_blob.data(), (char*) decompressed.data(), asset_file.binary_blob.size(), (int32) (decompressed.size()));

    mesh_cpu.vertices.rebsize(mesh_info.vertices_bsize);
    mesh_cpu.indices.rebsize(mesh_info.indices_bsize);

    memcpy(mesh_cpu.vertices.data(), decompressed.data(), mesh_info.vertices_bsize);
    memcpy(mesh_cpu.indices.data(), decompressed.data() + mesh_info.vertices_bsize, mesh_info.indices_bsize);

    return mesh_cpu;
}

void save_mesh(const MeshCPU& mesh_cpu) {
    AssetFile file;
    file.file_path = mesh_cpu.file_path;

    MeshInfo mesh_info;
    mesh_info.vertices_bsize = mesh_cpu.vertices.bsize();
    mesh_info.indices_bsize  = mesh_cpu.indices.bsize();
    mesh_info.index_bsize    = sizeof(uint32);

    vector<uint8> merged_buffer;
    merged_buffer.resize(uint32(mesh_info.vertices_bsize + mesh_info.indices_bsize));
    memcpy(merged_buffer.data(), mesh_cpu.vertices.data(), mesh_info.vertices_bsize);
    memcpy(merged_buffer.data() + mesh_info.vertices_bsize, mesh_cpu.indices.data(), mesh_info.indices_bsize);

    int32 compress_staging = LZ4_compressBound(int32(mesh_info.vertices_bsize + mesh_info.indices_bsize));
    file.binary_blob.resize(compress_staging);
    int32 compressed_bsize = LZ4_compress_default((char*) merged_buffer.data(), (char*) file.binary_blob.data(), int32(merged_buffer.size()), int32(compress_staging));
    file.binary_blob.resize(compressed_bsize);

    json j;
    j["mesh_cpu"] = make_shared<json_value>(to_jv(mesh_cpu));
    j["mesh_info"] = make_shared<json_value>(to_jv(mesh_info));
    file.asset_json = j;

    save_asset_file(file);
}

}
