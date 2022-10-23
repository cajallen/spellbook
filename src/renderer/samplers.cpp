#include "samplers.hpp"

namespace spellbook {

Sampler& Sampler::filter(Filter input) {
    filter_type = input;
    return *this;
}

Sampler& Sampler::address(Address input) {
    address_mode = input;
    return *this;
}

Sampler& Sampler::anisotropy(bool enable) {
    is_anisotropic = enable;
    return *this;
}

Sampler& Sampler::mips(bool enable) {
    mipped = enable;
    return *this;
}

vuk::SamplerCreateInfo Sampler::get() const {
    vuk::SamplerCreateInfo sci;

    switch (filter_type) {
        case (Filter_Linear):
            sci.magFilter = vuk::Filter::eLinear;
            sci.mipmapMode = vuk::SamplerMipmapMode::eLinear;
            break;
        case (Filter_Nearest):
            sci.magFilter = vuk::Filter::eNearest;
            sci.mipmapMode = vuk::SamplerMipmapMode::eNearest;
            break;
        case (Filter_Cubic):
            sci.magFilter = vuk::Filter::eCubicIMG;
            sci.mipmapMode = vuk::SamplerMipmapMode::eLinear;
            break;
    }
    sci.minFilter = sci.magFilter;

    switch (address_mode) {
        case (Address_Repeat):
            sci.addressModeU = vuk::SamplerAddressMode::eRepeat;
            sci.addressModeV = vuk::SamplerAddressMode::eRepeat;
            sci.addressModeW = vuk::SamplerAddressMode::eRepeat;
            break;
        case (Address_Clamp):
            sci.addressModeU = vuk::SamplerAddressMode::eClampToEdge;
            sci.addressModeV = vuk::SamplerAddressMode::eClampToEdge;
            sci.addressModeW = vuk::SamplerAddressMode::eClampToEdge;
            break;
        case (Address_Mirrored):
            sci.addressModeU = vuk::SamplerAddressMode::eMirroredRepeat;
            sci.addressModeV = vuk::SamplerAddressMode::eMirroredRepeat;
            sci.addressModeW = vuk::SamplerAddressMode::eMirroredRepeat;
            break;
        case (Address_Border):
            sci.addressModeU = vuk::SamplerAddressMode::eClampToBorder;
            sci.addressModeV = vuk::SamplerAddressMode::eClampToBorder;
            sci.addressModeW = vuk::SamplerAddressMode::eClampToBorder;
            break;
    }

    if (is_anisotropic) {
        sci.anisotropyEnable = VK_TRUE;
        sci.maxAnisotropy    = 16.f;
    } else {
        sci.anisotropyEnable = VK_FALSE;
    }

    if (mipped) {
    } else {
        sci.maxLod = 0.f;
    }

    return sci;
}

}
