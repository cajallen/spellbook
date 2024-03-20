﻿#include "file_browser.hpp"

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <stb_image.h>

#include "extension/imgui_extra.hpp"
#include "extension/icons/font_awesome4.h"

#include "general/umap.hpp"
#include "game/game.hpp"
#include "renderer/assets/model.hpp"
#include "renderer/assets/texture.hpp"

namespace spellbook {

void file_browser(const string& window_name, bool* p_open, FilePath* out) {
    ImGui::Begin(window_name.c_str(), p_open);

    std::function<bool(const FilePath&)> filter = [](const FilePath&){ return true; };

    bool popup_open = false;
    FilePath popup_set_input = {};
    FilePath popup_set_folder = {};
    string popup_set_name = "";

    std::function context_callback = [&popup_set_folder, &popup_set_name, &popup_set_input, &popup_open](const FilePath& path) -> void {
        if (ModelExternal::path_filter()(path)) {
            if (ImGui::Selectable("Convert")) {
                popup_set_input = FilePath(path);
                popup_set_folder = ModelCPU::folder();
                popup_set_name = popup_set_input.stem();
                popup_open = true;
            }
        }
        if (TextureExternal::path_filter()(path)) {
            if (ImGui::Selectable("Convert")) {
                popup_set_input = FilePath(path);
                popup_set_folder = TextureCPU::folder();
                popup_set_name = popup_set_input.stem();
                popup_open = true;
            }
        }
    };

    // Header
    {
        if (ImGui::Button(ICON_FA_CHEVRON_UP)) {
            *out = FilePath(out->abs_path().parent_path());
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-200.f);
        string out_as_string = out->rel_string();
        bool changed = ImGui::InputText("##Current", &out_as_string, ImGuiInputTextFlags_ReadOnly);
        if (changed)
            *out = FilePath(out_as_string);
        //ImGui::SameLine();
        //ImGui::SetNextItemWidth(160.f);
        //ImGui::EnumCombo("Type", &asset_type_map[window_name]);
    }

    // Body
    ImGui::PathSelectBody(out, Directory::path_filter()(*out) ? *out : FilePath(out->abs_path().parent_path()), filter, nullptr, 1, context_callback);


    // Popup
    if (popup_open) {
        if (ModelExternal::path_filter()(popup_set_input))
            ImGui::OpenPopup("Convert Model Asset");
        if (TextureExternal::path_filter()(popup_set_input))
            ImGui::OpenPopup("Convert Texture Asset");

    }

    if (ImGui::BeginPopupModal("Convert Model Asset")) {
        struct ModelConvertInfo {
            FilePath input;
            FilePath folder_path;
            string name;
            bool y_up = true;
        };
        static umap<string, ModelConvertInfo> model_convert_map;

        if (!model_convert_map.contains(window_name))
            model_convert_map[window_name] = {};

        if (popup_open) {
            model_convert_map[window_name].input = popup_set_input;
            model_convert_map[window_name].folder_path = popup_set_folder;
            model_convert_map[window_name].name = popup_set_name;
        }

        ImGui::PathSelect<ModelCPU>("Input", &model_convert_map[window_name].input);
        ImGui::PathSelect<Directory>("Output folder", &model_convert_map[window_name].folder_path);
        ImGui::InputText("Output name", &model_convert_map[window_name].name);
        ImGui::Checkbox("Y-Up", &model_convert_map[window_name].y_up);
        if (ImGui::Button("Convert")) {
            auto& convert_info = model_convert_map[window_name];
            save_resource(convert_to_model(convert_info.input, convert_info.folder_path, convert_info.name, convert_info.y_up));
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Convert Texture Asset")) {
        struct TextureConvertInfo {
            FilePath input;
            FilePath folder_path;
            string name;
        };
        static umap<string, TextureConvertInfo> tex_convert_map;

        if (!tex_convert_map.contains(window_name))
            tex_convert_map[window_name] = {};

        if (popup_open) {
            tex_convert_map[window_name].input = popup_set_input;
            tex_convert_map[window_name].folder_path = popup_set_folder;
            tex_convert_map[window_name].name = popup_set_name;
        }

        ImGui::PathSelect<ModelCPU>("Input", &tex_convert_map[window_name].input);
        ImGui::PathSelect<Directory>("Output folder", &tex_convert_map[window_name].folder_path);
        ImGui::InputText("Output name", &tex_convert_map[window_name].name);


        if (ImGui::Button("Convert")) {
            auto& convert_info = tex_convert_map[window_name];

            string abs_path = convert_info.input.abs_string();
            v2i size;
            auto pixels = stbi_load(abs_path.c_str(), &size.x, &size.y, nullptr, 4);

            TextureCPU texture_cpu = {
                FilePath(convert_info.folder_path.rel_string() + convert_info.name + string(TextureCPU::extension())),
                {},
                v2i{size.x, size.y},
                vuk::Format::eR8G8B8A8Srgb,
                vector<uint8>(pixels, pixels + size.x * size.y * 4)
            };
            save_texture(texture_cpu);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }


    ImGui::End();
}



}