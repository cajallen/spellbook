#include "json_cache.hpp"

#include "editor/console.hpp"
#include "extension/fmt.hpp"
#include "game/game_file.hpp"

namespace spellbook {

json& DiskCache::load_json(const FilePath& file_path) {
    if (parsed_jsons.contains(file_path))
        return parsed_jsons[file_path];

    console({.str=fmt_("Loading json: {}", file_path.rel_string()), .group = "asset", .save = false});
    parsed_jsons[file_path] = parse_file(file_path.abs_string());
    load_dependencies(parsed_jsons[file_path]);
    return parsed_jsons[file_path];
}

AssetFile& DiskCache::load_asset(const FilePath& file_path) {
    // check for fake file path
    if (parsed_assets.contains(file_path))
        return parsed_assets[file_path];
    console({.str=fmt_("Loading asset: {}", file_path.rel_string()), .group = "asset", .save = false});
    return parsed_assets[file_path] = load_asset_file(file_path);
}

vector<FilePath> DiskCache::load_dependencies(json& j) {
    vector<FilePath> list;
    if (j.contains("dependencies")) {
        for (const json_value& jv : j["dependencies"]->get_list()) {
            FilePath& file_path = list.push_back(FilePath(from_jv_impl(jv, (string*) 0)));

            FileCategory category = get_category(file_path);
            if (category == FileCategory_Asset)
                this->load_asset(file_path);
            if (category == FileCategory_Json)
                this->load_json(file_path);
        }
    }
    return list;
}

}
