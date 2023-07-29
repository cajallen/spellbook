#include "texture.hpp"

#include "game/game.hpp"
#include "general/logger.hpp"
#include "vuk/Partials.hpp"

namespace spellbook {

FilePath upload_texture(const TextureCPU& tex_cpu, bool frame_allocation) {
    assert_else(tex_cpu.file_path.is_file());
    uint64 tex_cpu_hash = hash_path(tex_cpu.file_path);
    vuk::Allocator& alloc = frame_allocation ? *game.renderer.frame_allocator : *game.renderer.global_allocator;
    auto [tex, tex_fut] = vuk::create_texture(alloc, tex_cpu.format, vuk::Extent3D(tex_cpu.size), (void*) tex_cpu.pixels.data(), true);
    game.renderer.context->set_name(tex, vuk::Name(tex_cpu.file_path.rel_string()));
    game.renderer.enqueue_setup(std::move(tex_fut));
    
    game.renderer.texture_cache[tex_cpu_hash] = {std::move(tex), frame_allocation};
    return tex_cpu.file_path;
}

}
