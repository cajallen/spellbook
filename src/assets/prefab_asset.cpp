#include "prefab_asset.hpp"

#include <lz4.h>

using namespace caj;

assets::PrefabInfo assets::read_prefab_info(AssetFile* file) {
    return PrefabInfo(json_value(parse(file->json)));
}

assets::AssetFile assets::pack_prefab(PrefabInfo* info) {
    // core file header
    AssetFile file;
    file.type[0] = 'P';
    file.type[1] = 'R';
    file.type[2] = 'F';
    file.type[3] = 'B';
    file.version = 1;

    file.json = ((json_value) *info).dump();

    return file;
}
