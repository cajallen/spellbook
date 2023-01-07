#pragma once

#include "general/string.hpp"
#include "samplers.hpp"

namespace spellbook {

struct Image {
    string texture;
    Sampler sampler;
};

}