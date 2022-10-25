#include "asset_browser.hpp"

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "lib_ext/imgui_extra.hpp"

#include "game.hpp"
#include "umap.hpp"
#include "renderer/assets/asset_loader.hpp"

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

void asset_browser(const string& window_name, bool* p_open, fs::path* out) {
    static umap<string, AssetType> asset_type_map;

    ImGui::Begin(window_name.c_str(), p_open);

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

    bool popup_open = false;
    fs::path popup_set_input = "";
    fs::path popup_set_folder = "";
    string popup_set_name = "";

    std::function context_callback = [&popup_set_folder, &popup_set_name, &popup_set_input, &popup_open](const fs::path& path) -> void {
        if (possible_model_asset(path)) {
            if (ImGui::Selectable("Convert")) {
                popup_set_input = path;
                popup_set_folder = path.parent_path();
                popup_set_name = path.stem().string();
                popup_open = true;
            }
        }
        if (possible_texture_asset(path)) {
            if (ImGui::Selectable("Convert")) {
                
            }
        }
    };

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
    PathSelectBody(out, possible_folder(*out) ? *out : out->parent_path(), filter, nullptr, false, context_callback);
    

    // Popup
    if (popup_open)
        ImGui::OpenPopup("Convert Model Asset");
    if (ImGui::BeginPopupModal("Convert Model Asset")) {
        struct ModelConvertInfo {
            fs::path input;
            fs::path folder_path;
            string name;
        };
        static umap<string, ModelConvertInfo> model_convert_map;
        
        if (!model_convert_map.contains(window_name))
            model_convert_map[window_name] = {};

        if (!popup_set_input.empty())
            model_convert_map[window_name].input = popup_set_input;
        if (!popup_set_folder.empty())
            model_convert_map[window_name].folder_path = popup_set_folder.lexically_proximate(game.external_resource_folder);
        if (!popup_set_name.empty())
            model_convert_map[window_name].name = popup_set_name;

        PathSelect("Input", &model_convert_map[window_name].input, game.external_resource_folder, possible_model_asset, "DND_MODEL_ASSET");
        PathSelect("Output folder", &model_convert_map[window_name].folder_path, game.resource_folder, possible_folder, "DND_FOLDER");
        ImGui::InputText("Output name", &model_convert_map[window_name].name);
        if (ImGui::Button("Convert")) {
            save_model(convert_to_model(model_convert_map[window_name].input, model_convert_map[window_name].folder_path, model_convert_map[window_name].name));
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    
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

bool possible_tower(const fs::path& path) {
    return path.extension().string() == tower_extension;
}




}
