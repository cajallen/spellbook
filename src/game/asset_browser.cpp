#include "asset_browser.hpp"

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "math.hpp"
#include "lib_ext/imgui_extra.hpp"

#include "umap.hpp"

namespace spellbook {

enum AssetType {
    AssetType_Any,
    AssetType_Model,
    AssetType_ModelAsset,
    AssetType_Texture,
    AssetType_TextureAsset,
    AssetType_Material,
    AssetType_Mesh,
    AssetType_Count
};

void asset_browser(const string& window_name, fs::path* out) {
    static umap<string, AssetType> asset_type_map;

    ImGui::Begin(window_name.c_str());

    if (!asset_type_map.contains(window_name))
        asset_type_map[window_name] = AssetType_Any;

    std::function<bool(const fs::path&)> filter;
    switch (asset_type_map[window_name]) {
        case (AssetType_Any): {
            filter = [](const fs::path&) { return true; };
        } break;
        case (AssetType_Model): {
            filter = [](const fs::path& path) { return possible_model(path); };
        } break;
        case (AssetType_ModelAsset): {
            filter = [](const fs::path& path) { return possible_model_asset(path); };
        } break;
        case (AssetType_Texture): {
            filter = [](const fs::path& path) { return possible_texture(path); };
        } break;
        case (AssetType_TextureAsset): {
            filter = [](const fs::path& path) { return possible_texture_asset(path); };
        } break;
        case (AssetType_Material): {
            filter = [](const fs::path& path) { return possible_material(path); };
        } break;
        case (AssetType_Mesh): {
            filter = [](const fs::path& path) { return possible_mesh(path); };
        } break;
    }

    // Header
    {
        if (ImGui::Button("^")) {
            *out = out->parent_path();
        }
        ImGui::SameLine();
        string out_as_string = out->string();
        ImGui::SetNextItemWidth(-100.f);
        ImGui::InputText("##Current", &out_as_string, ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60.f);
        ImGui::Combo("Type", (s32*) &asset_type_map[window_name], "Any\0Model\0Model Asset\0Texture\0Texture Asset\0Material\0Mesh\0\0");
    }

    // Body
    PathSelectBody(out, possible_folder(*out) ? *out : out->parent_path(), filter, nullptr, false);
    
    
    ImGui::End();
}


bool possible_file(const fs::path& path) {
    return is_regular_file(path);
}

bool possible_folder(const fs::path& path) {
    return is_directory(path);
}

bool possible_model(const fs::path& path) {
    return path.extension().string() == model_extension;
}

bool possible_material(const fs::path& path) {
    return path.extension().string() == material_extension;
}

bool possible_texture(const fs::path& path) {
    return path.extension().string() == texture_extension;
}

bool possible_mesh(const fs::path& path) {
    return path.extension().string() == mesh_extension;
}

bool possible_model_asset(const fs::path& path) {
    return vector<string>{".gltf", ".glb"}.contains(path.extension().string());
}

bool possible_mesh_asset(const fs::path& path) {
    return false;
}

bool possible_texture_asset(const fs::path& path) {
    return vector<string>{".png", ".jpg", ".jpeg"}.contains(path.extension().string());
}



}
