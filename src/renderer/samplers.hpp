#pragma once

#include <vuk/Image.hpp>

namespace spellbook {

// Commonly used sampler presets.
constexpr auto LinearClamp = vuk::SamplerCreateInfo{
	.magFilter = vuk::Filter::eLinear,
	.minFilter = vuk::Filter::eLinear,
	.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
	.addressModeV = vuk::SamplerAddressMode::eClampToEdge };

constexpr auto TrilinearClamp = vuk::SamplerCreateInfo{
	.magFilter = vuk::Filter::eLinear,
	.minFilter = vuk::Filter::eLinear,
	.mipmapMode = vuk::SamplerMipmapMode::eLinear,
	.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
	.addressModeV = vuk::SamplerAddressMode::eClampToEdge };

constexpr auto NearestClamp = vuk::SamplerCreateInfo{
	.magFilter = vuk::Filter::eNearest,
	.minFilter = vuk::Filter::eNearest,
	.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
	.addressModeV = vuk::SamplerAddressMode::eClampToEdge };

constexpr auto TrilinearMirror = vuk::SamplerCreateInfo {
	.magFilter = vuk::Filter::eLinear,
	.minFilter = vuk::Filter::eLinear,
	.mipmapMode = vuk::SamplerMipmapMode::eLinear,
    .addressModeU = vuk::SamplerAddressMode::eMirroredRepeat,
    .addressModeV = vuk::SamplerAddressMode::eMirroredRepeat
};

constexpr auto LinearMirrorNoMip = vuk::SamplerCreateInfo {
	.magFilter = vuk::Filter::eLinear,
	.minFilter = vuk::Filter::eLinear,
    .addressModeU = vuk::SamplerAddressMode::eMirroredRepeat,
    .addressModeV = vuk::SamplerAddressMode::eMirroredRepeat,
    .maxLod = 0.f
};

constexpr auto TrilinearAnisotropic = vuk::SamplerCreateInfo{
	.magFilter = vuk::Filter::eLinear,
	.minFilter = vuk::Filter::eLinear,
	.mipmapMode = vuk::SamplerMipmapMode::eLinear,
	.addressModeU = vuk::SamplerAddressMode::eRepeat,
	.addressModeV = vuk::SamplerAddressMode::eRepeat,
    .anisotropyEnable = VK_TRUE,
    .maxAnisotropy	= 16.0f
};

}
