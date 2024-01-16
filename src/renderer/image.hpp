#pragma once

#include "general/file/file_path.hpp"
#include "renderer/samplers.hpp"

namespace spellbook {

struct Image {
    FilePath texture;
    Sampler sampler;
};

}