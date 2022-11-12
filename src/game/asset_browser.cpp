#include "asset_browser.hpp"

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "lib_ext/imgui_extra.hpp"

#include "game.hpp"
#include "umap.hpp"
#include "file.hpp"
#include "lib_ext/icons/font_awesome4.h"
#include "renderer/assets/asset_loader.hpp"
#include "renderer/assets/texture_asset.hpp"

namespace spellbook {


void asset_browser(const string& window_name, bool* p_open, fs::path* out) {
    static umap<string, FileType> asset_type_map;

    ImGui::Begin(window_name.c_str(), p_open);

    if (!asset_type_map.contains(window_name))
        asset_type_map[window_name] = FileType_Unknown;

    std::function<bool(const fs::path&)> filter = path_filter(asset_type_map[window_name]);

    bool popup_open = false;
    fs::path popup_set_input = "";
    fs::path popup_set_folder = "";
    string popup_set_name = "";

    std::function context_callback = [&popup_set_folder, &popup_set_name, &popup_set_input, &popup_open](const fs::path& path) -> void {
        if (path_filter(FileType_ModelAsset)(path)) {
            if (ImGui::Selectable("Convert")) {
                popup_set_input = path;
                popup_set_folder = path.parent_path();
                popup_set_name = path.stem().string();
                popup_open = true;
            }
        }
        if (path_filter(FileType_TextureAsset)(path)) {
            if (ImGui::Selectable("Convert")) {
                popup_set_input = path;
                popup_set_folder = path.parent_path();
                popup_set_name = path.stem().string();
                popup_open = true;
            }
        }
    };

    // Header
    {
        if (ImGui::Button(ICON_FA_CHEVRON_UP)) {
            *out = out->parent_path();
        }
        ImGui::SameLine();
        string out_as_string = out->string();
        ImGui::SetNextItemWidth(-200.f);
        ImGui::InputText("##Current", &out_as_string, ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(160.f);
        ImGui::EnumCombo("Type", &asset_type_map[window_name]);
    }

    // Body
    ImGui::PathSelectBody(out, path_filter(FileType_Directory)(*out) ? *out : out->parent_path(), filter, nullptr, false, context_callback);
    

    // Popup
    if (popup_open) {
        if (path_filter(FileType_ModelAsset)(popup_set_input))
            ImGui::OpenPopup("Convert Model Asset");
        if (path_filter(FileType_TextureAsset)(popup_set_input))
            ImGui::OpenPopup("Convert Texture Asset");
        
    }
    
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

        ImGui::PathSelect("Input", &model_convert_map[window_name].input, game.external_resource_folder, FileType_ModelAsset);
        ImGui::PathSelect("Output folder", &model_convert_map[window_name].folder_path, game.resource_folder, FileType_Directory);
        ImGui::InputText("Output name", &model_convert_map[window_name].name);
        if (ImGui::Button("Convert")) {
            save_model(convert_to_model(model_convert_map[window_name].input, model_convert_map[window_name].folder_path, model_convert_map[window_name].name));
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Convert Texture Asset")) {
        struct TextureConvertInfo {
            fs::path input;
            fs::path folder_path;
            string name;
        };
        static umap<string, TextureConvertInfo> texture_convert_map;
        
        if (!texture_convert_map.contains(window_name))
            texture_convert_map[window_name] = {};

        if (!popup_set_input.empty())
            texture_convert_map[window_name].input = popup_set_input;
        if (!popup_set_folder.empty())
            texture_convert_map[window_name].folder_path = popup_set_folder.lexically_proximate(game.external_resource_folder);
        if (!popup_set_name.empty())
            texture_convert_map[window_name].name = popup_set_name;

        ImGui::PathSelect("Input", &texture_convert_map[window_name].input, game.external_resource_folder, FileType_ModelAsset);
        ImGui::PathSelect("Output folder", &texture_convert_map[window_name].folder_path, game.resource_folder, FileType_Directory);
        ImGui::InputText("Output name", &texture_convert_map[window_name].name);
        if (ImGui::Button("Convert")) {
            save_texture(convert_to_texture(texture_convert_map[window_name].input.string(), texture_convert_map[window_name].folder_path.string(), texture_convert_map[window_name].name));
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    
    ImGui::End();
}




}
