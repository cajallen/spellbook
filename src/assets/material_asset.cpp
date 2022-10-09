#include "material_asset.hpp"

#include <lz4.h>

namespace spellbook::assets {

MaterialInfo read_material_info(AssetFile* file) {
    return MaterialInfo(json_value(parse(file->json)));
}

AssetFile pack_material(MaterialInfo* info) {
    // core file header
    AssetFile file;
    file.type[0] = 'M';
    file.type[1] = 'A';
    file.type[2] = 'T';
    file.type[3] = 'X';
    file.version = 1;

    file.json = ((json_value) *info).dump();

    return file;
}

}
