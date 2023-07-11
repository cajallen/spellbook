#include "mesh.hpp"

#include <imgui/imgui.h>
#include <vuk/Partials.hpp>

#include "extension/fmt.hpp"
#include "general/math.hpp"
#include "general/logger.hpp"
#include "game/game.hpp"


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

u64 upload_mesh(const MeshCPU& mesh_cpu, bool frame_allocation) {
    if (mesh_cpu.file_path.empty())
        return 0;
    u64 mesh_cpu_hash           = hash_string(mesh_cpu.file_path);
    if (game.renderer.mesh_cache.contains(mesh_cpu_hash))
        return mesh_cpu_hash;
    MeshGPU         mesh_gpu;
    mesh_gpu.frame_allocated = frame_allocation;
    vuk::Allocator& alloc                = frame_allocation ? *game.renderer.frame_allocator : *game.renderer.global_allocator;
    auto            [vert_buf, vert_fut] = vuk::create_buffer(alloc, vuk::MemoryUsage::eGPUonly, vuk::DomainFlagBits::eTransferOnTransfer, std::span(mesh_cpu.vertices));
    mesh_gpu.vertex_buffer               = std::move(vert_buf);
    auto [idx_buf, idx_fut]              = vuk::create_buffer(alloc, vuk::MemoryUsage::eGPUonly, vuk::DomainFlagBits::eTransferOnTransfer, std::span(mesh_cpu.indices));
    mesh_gpu.index_buffer                = std::move(idx_buf);
    mesh_gpu.index_count                 = mesh_cpu.indices.size();
    mesh_gpu.vertex_count                = mesh_cpu.vertices.size();

    game.renderer.enqueue_setup(std::move(vert_fut));
    game.renderer.enqueue_setup(std::move(idx_fut));

    game.renderer.mesh_cache[mesh_cpu_hash] = std::move(mesh_gpu);
    game.renderer.file_path_cache[mesh_cpu_hash] = mesh_cpu.file_path;
    return mesh_cpu_hash;
}

}
