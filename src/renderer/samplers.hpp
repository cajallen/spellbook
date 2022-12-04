#pragma once

#include <vuk/Image.hpp>

#include "general/json.hpp"

namespace spellbook {

enum Filter { Filter_Linear, Filter_Nearest, Filter_Cubic };
enum Address { Address_Repeat, Address_Clamp, Address_Mirrored, Address_Border };

struct Sampler {
    Filter filter_type = Filter_Linear;
    Address address_mode = Address_Repeat;
    bool is_anisotropic = false;
    bool mipped = true;
    
    Sampler& filter(Filter input);
    Sampler& address(Address input);
    Sampler& anisotropy(bool enable = true);
    Sampler& mips(bool enable = true);
    vuk::SamplerCreateInfo get() const;
};

JSON_IMPL(Sampler, filter_type, address_mode, is_anisotropic, mipped);

}
