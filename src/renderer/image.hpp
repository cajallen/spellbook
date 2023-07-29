#pragma once

#include "renderer/samplers.hpp"
#include "general/file_path.hpp"

namespace spellbook {

struct Image {
    FilePath texture;
    Sampler sampler;
};

}