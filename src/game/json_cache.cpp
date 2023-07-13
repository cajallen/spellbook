#include "json_cache.hpp"

#include "editor/console.hpp"
#include "extension/fmt.hpp"
#include "game/game_file.hpp"

namespace spellbook {

json& AssetCache::load_json(const string& file_path_input) {
    string file_path = file_path_input;
    std::replace(file_path.begin(), file_path.end(), '\\', '/');
    if (parsed_jsons.contains(file_path))
        return parsed_jsons[file_path];

    console({.str=fmt_("Loading json: {}", file_path), .group = "asset", .save = false});
    parsed_jsons[file_path] = parse_file(file_path);
    load_dependencies(parsed_jsons[file_path]);
    return parsed_jsons[file_path];
}

AssetFile& AssetCache::load_asset(const string& file_path_input) {
    string file_path = file_path_input;
    std::replace(file_path.begin(), file_path.end(), '\\', '/');
    if (parsed_assets.contains(file_path))
        return parsed_assets[file_path];

    console({.str=fmt_("Loading asset: {}", file_path), .group = "asset", .save = false});
    return parsed_assets[file_path] = load_asset_file(file_path);
}

vector<string> AssetCache::load_dependencies(json& j) {
    vector<string> list;
    if (j.contains("dependencies")) {
        for (const json_value& jv : j["dependencies"]->get_list()) {
            string& file_path = list.push_back(from_jv_impl(jv, (string*) 0));
            
            FileCategory category = file_category(file_type_from_path(fs::path(file_path)));
            if (category == FileCategory_Asset)
                this->load_asset(to_resource_path(file_path).string());
            if (category == FileCategory_Json)
                this->load_json(to_resource_path(file_path).string());
        }
    }
    return list;
}

}
