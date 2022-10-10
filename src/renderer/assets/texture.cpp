#include "texture.hpp"

#include <hash.hpp>

#include "console.hpp"

#include "renderer/assets/asset_loader.hpp"
#include "renderer/assets/texture_asset.hpp"

namespace spellbook {

u64 TextureCPU::contents_hash() const {
    auto hash1 = hash_data(pixels.data(), pixels.bsize());
    auto hash2 = hash_data(&format, sizeof(format));
    auto hash3 = hash_data(&size, sizeof(size));
    return hash1 ^ hash2 ^ hash3; // sue me
}


}