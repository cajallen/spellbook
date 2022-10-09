#pragma once

#include <vuk/Types.hpp>

#include "string.hpp"

#include "geometry.hpp"

#include "renderer/render_scene.hpp"

namespace spellbook {

struct TextureCPU {
    string        name = {};
    v2i           size = {};
    vuk::Format format = {};
    u8*           data = nullptr;
};

using TextureGPU = vuk::Texture;

void       save_texture(const TextureCPU&);
TextureCPU load_texture(const string_view file_name);

}
