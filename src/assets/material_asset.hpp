#pragma once
#include "asset_loader.hpp"

#include "umap.hpp"
#include "json.hpp"

#include "string.hpp"

namespace spellbook::assets {

enum TransparencyMode { TransparencyMode_Opaque, TransparencyMode_Transparent, TransparencyMode_Masked };

struct MaterialInfo {
    string               base_effect;
    umap<string, string> textures; // name -> path
    umap<string, f32>    properties;
    u32                  transparency; // TransparencyMode

    MaterialInfo() = default;

    explicit MaterialInfo(const json_value& jv) {
        json& j = (json&) jv;
        if (j.count("base_effect") == 1) {
            base_effect = (string) *j.at("base_effect");
        }
        if (j.count("textures") == 1) {
            json_value& jv = *j.at("textures");
            textures       = (umap<string, string>) jv;
        }
        if (j.count("properties") == 1) {
            using JsonT = decltype(properties);
            properties = (JsonT) *j.at("properties");
        }
        if (j.count("transparency") == 1) {
            using JsonT = decltype(transparency);
            transparency = (JsonT) *j.at("transparency");
        }
    }

    explicit operator json_value() const {
        json j            = json{};
        j["base_effect"]  = make_shared<json_value>(json_value(base_effect));
        j["textures"]     = make_shared<json_value>(json_value(textures));
        j["properties"]   = make_shared<json_value>(json_value(properties));
        j["transparency"] = make_shared<json_value>(json_value(transparency));
        return json_value(j);
    }

    JSON_IMPL(MaterialInfo, base_effect, textures, properties, transparency);
};

MaterialInfo read_material_info(AssetFile * file);
AssetFile    pack_material(MaterialInfo* info);

}
