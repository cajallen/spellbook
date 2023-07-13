#include "visual_tile.hpp"

#include <Tracy/Tracy.hpp>
#include <entt/entity/registry.hpp>

#include "extension/fmt.hpp"
#include "extension/imgui_extra.hpp"
#include "extension/icons/font_awesome4.h"
#include "general/logger.hpp"
#include "general/math/math.hpp"
#include "general/math/matrix_math.hpp"
#include "game/scene.hpp"
#include "renderer/draw_functions.hpp"
#include "editor/widget_system.hpp"


namespace spellbook {

VisualTileCorners apply_rotation(VisualTileCorners corners, VisualTileRotation rotation) {
    if (rotation.flip_x) {
        std::swap(corners[NNN], corners[PNN]);
        std::swap(corners[NPN], corners[PPN]);
        std::swap(corners[NNP], corners[PNP]);
        std::swap(corners[NPP], corners[PPP]);
    }
    if (rotation.flip_z) {
        std::swap(corners[NNN], corners[NNP]);
        std::swap(corners[NPN], corners[NPP]);
        std::swap(corners[PNN], corners[PNP]);
        std::swap(corners[PPN], corners[PPP]);
    }
    while (rotation.yaw > 0) {
        uint8 backup_origin = corners[NNN];
        corners[NNN] = corners[NPN];
        corners[NPN] = corners[PPN];
        corners[PPN] = corners[PNN];
        corners[PNN] = backup_origin;

        backup_origin = corners[NNP];
        corners[NNP] = corners[NPP];
        corners[NPP] = corners[PPP];
        corners[PPP] = corners[PNP];
        corners[PNP] = backup_origin;
        rotation.yaw--;
    }
    return corners;
}

bool get_rotation(VisualTileCorners corners, VisualTileCorners target, VisualTileRotation& out_rotation, uint32 seed, bool flip_z) {
    vector<VisualTileRotation> candidate_rotations = {};

    for (uint8 i = 0; i <= (flip_z ? 0b1111 : 0b111); i++) {
        VisualTileRotation rotation {
            .yaw = uint8(i & 0b11),
            .flip_x = bool(i & 0b0100),
            .flip_z = bool(i & 0b1000),
        };
        if (apply_rotation(corners, rotation) == target)
            candidate_rotations.push_back(rotation);
    }
    
    if (candidate_rotations.empty())
        return false;

    math::random_seed(seed);
    out_rotation = candidate_rotations[math::random_int32(candidate_rotations.size())];
    return true;
}

umap<v3i, VisualTileEntry> build_visual_tiles(umap<v3i, uint8>& solids, const umap<VisualTileCorners, vector<string>>& entry_pool, v3i* single_tile) {
    umap<v3i, VisualTileEntry> entries;

    if (single_tile) {
        for (int x = 0; x <= 1; ++x) {
            for (int y = 0; y <= 1; ++y) {
                for (int z = 0; z <= 1; ++z) {
                    entries[*single_tile - v3i(x,y,z)] = {};
                }
            } 
        }
    } else {
        for (const auto& [coord, _] : solids) {
            for (int x = -1; x <= 1; ++x) {
                for (int y = -1; y <= 1; ++y) {
                    for (int z = -1; z <= 1; ++z) {
                        entries[coord - v3i(x,y,z)] = {};
                    }
                } 
            }
        }
    }

    // TODO: add support for setting this seed
    // TODO: make the seeds local, rather than relying on the chaining.
    uint32 seed = 0;
    for (auto& [pos, entry] : entries) {
        VisualTileCorners tile_corners = {};
        for (int i = 0; i < 8; ++i) {
            v3i key = pos + visual_direction_offsets[i];
            tile_corners[i] = solids.contains(key) ? solids[key] : 0b1;
        }

        vector<VisualTileEntry> candidate_entries = {};
        for (auto& [entry_corners, entry_model] : entry_pool) {
            VisualTileRotation entry_rotation;
            bool viable = get_rotation(entry_corners, tile_corners, entry_rotation, seed++);
            if (viable) {
                math::random_seed(seed++);
                candidate_entries.emplace_back(entry_model[math::random_int32(entry_model.size())], entry_rotation);
            }
        }
        if (candidate_entries.empty()) {
            continue;
        }
        math::random_seed(seed++);
        entry = candidate_entries[math::random_int32(candidate_entries.size())];
    }
    
    return entries;
}

umap<VisualTileCorners, vector<string>> convert_to_entry_pool(const VisualTileSet& tile_set) {
    umap<VisualTileCorners, vector<string>> entry_pool;

    for (auto& tile : tile_set.tiles) {
        if (!entry_pool.contains(tile.corners))
            entry_pool[tile.corners] = {};
        entry_pool[tile.corners].push_back(tile.model_path);
    }
    return entry_pool;
}

bool inspect(VisualTileSet* tile_set) {
    bool changed = false;
    ImGui::PathSelect("File", &tile_set->file_path, "resources/visual_tile_sets", FileType_VisualTileSet);
    for (auto& tile : tile_set->tiles) {
        ImGui::Text("Entry");

        ImGui::PushID(&tile);
        ImGui::Indent();
        {
            int step = 1;
            ImGui::PushItemWidth(60.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {20, 2});
            ImGui::InputScalar("NNN", ImGuiDataType_U8, &tile.corners[NNN], &step);
            ImGui::SameLine();
            ImGui::InputScalar("PNN", ImGuiDataType_U8, &tile.corners[PNN], &step);
            ImGui::SameLine();
            ImGui::Dummy(ImVec2{35, 1});
            ImGui::SameLine();
            ImGui::InputScalar("NNP", ImGuiDataType_U8, &tile.corners[NNP], &step);
            ImGui::SameLine();
            ImGui::InputScalar("PNP", ImGuiDataType_U8, &tile.corners[PNP], &step);

            ImGui::SameLine();
            ImGui::Dummy(ImVec2{35, 1});
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_TIMES)) {
                tile_set->tiles.remove_index(tile_set->tiles.index(tile));
                ImGui::PopStyleVar();
                ImGui::PopItemWidth();
                ImGui::Unindent();
                ImGui::PopID();
                break;
            }
            
            ImGui::InputScalar("NPN", ImGuiDataType_U8, &tile.corners[NPN], &step);
            ImGui::SameLine();
            ImGui::InputScalar("PPN", ImGuiDataType_U8, &tile.corners[PPN], &step);
            ImGui::SameLine();
            ImGui::Dummy(ImVec2{35, 1});
            ImGui::SameLine();
            ImGui::InputScalar("NPP", ImGuiDataType_U8, &tile.corners[NPP], &step);
            ImGui::SameLine();
            ImGui::InputScalar("PPP", ImGuiDataType_U8, &tile.corners[PPP], &step);
            ImGui::PopStyleVar();
            ImGui::PopItemWidth();

            changed |= ImGui::PathSelect("Model", &tile.model_path, "resources/models", FileType_Model, true);

            if (ImGui::Button("Rotate 1")) {
                tile.corners = apply_rotation(tile.corners, VisualTileRotation{.yaw = 1, .flip_x = false});
            }
            ImGui::SameLine();
            if (ImGui::Button("Flip")) {
                tile.corners = apply_rotation(tile.corners, VisualTileRotation{.yaw = 0, .flip_x = true});
            }

        }
        ImGui::Unindent();
        ImGui::PopID();
    }
    if (ImGui::Button(ICON_FA_PLUS, ImVec2{100, 20})) {
        tile_set->tiles.emplace_back();
    }
    ImGui::SameLine();

    if (ImGui::Button("Folder"))
        ImGui::OpenPopup("Convert Folder");
    
    bool discard = true;
    if (ImGui::BeginPopupModal("Convert Folder", &discard)) {
        static umap<VisualTileSet*, fs::path> convert_paths;
        fs::path& convert_path = convert_paths.contains(tile_set) ? convert_paths[tile_set] : convert_paths[tile_set] = "resources/models/";
        ImGui::PathSelect("Folder", &convert_path, "resources/models", FileType_Model);
        if (ImGui::Button("Convert")) {
            for (auto& dir_entry : fs::directory_iterator(convert_path)) {
                if (path_filter(FileType_Model)(dir_entry)) {
                    bool already_exists = false;
                    for (auto& tile : tile_set->tiles) {
                        if (tile.model_path == dir_entry.path().string()) {
                            already_exists = true;
                            break;
                        }
                    }
                    if (!already_exists)
                        tile_set->tiles.emplace_back(dir_entry.path().string(), VisualTileCorners{});
                }
            }
            
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    return changed;
}

void visual_tile_widget_system(Scene* scene) {
    ZoneScoped;
    static uint64 mesh_id = 0;
    static uint64 mat_off_id;
    static uint64 mat_on1_id;
    static uint64 mat_on2_id;
    static uint64 mat_on3_id;
    static uint64 mat_on_id;
    static uint64 mat_err_id;
    if (mesh_id == 0) {
        mesh_id = upload_mesh(generate_icosphere(2), false);
        mat_off_id = upload_material({.file_path = "mat_off_name", .color_tint = palette::gray_1}, false);
        mat_on1_id = upload_material({.file_path = "mat_on1_name", .color_tint = palette::gray_1, .emissive_tint = Color::hsvf(0.60f, 0.3f, 0.8f)}, false);
        mat_on2_id = upload_material({.file_path = "mat_on2_name", .color_tint = palette::gray_1, .emissive_tint = Color::hsvf(0.08f, 0.7f, 0.6f)}, false);
        mat_on3_id = upload_material({.file_path = "mat_on3_name", .color_tint = palette::gray_1, .emissive_tint = Color::hsvf(0.02f, 0.7f, 0.3f)}, false);
        mat_on_id = upload_material({.file_path = "mat_on_name", .color_tint = palette::gray_1, .emissive_tint = palette::white}, false);
        mat_err_id = upload_material({.file_path = "mat_err_name", .color_tint = palette::gray_1, .emissive_tint = palette::red}, false);
    }
    for (auto [entity, vtsw] : scene->registry.view<VisualTileSetWidget>().each()) {
        if (vtsw.tile_set != nullptr) {
            for (int i = 0; i < 7; i++)
                if (Input::key_down[GLFW_KEY_1 + i])
                    vtsw.setting = i;
            
            auto& tiles = vtsw.tile_set->tiles;
            uint32 width = uint32(math::ceil(math::sqrt(float(tiles.size()))));
            uint32 i = 0;
            for (VisualTilePrefab& tile_entry : tiles) {
                v3 pos = (v3(i % width, i / width, 0.0f) - v3(0.5f * width, 0.5f * width, 0.0f)) * 3.0f;
                i++;
                for (int c = 0; c < 8; c++) {
                    uint64 mat_id;
                    if (tile_entry.corners[c] & (0b1 << vtsw.setting)) {
                        switch (vtsw.setting) {
                            case 0: mat_id = mat_on1_id; break;
                            case 1: mat_id = mat_on2_id; break;
                            case 2: mat_id = mat_on3_id; break;
                            default: mat_id = mat_on_id; break;
                        }
                    } else {
                        mat_id = mat_off_id;
                    }
                    if (tile_entry.corners[c] == 0)
                        mat_id = mat_err_id;

                    auto& r = scene->render_scene.quick_renderable(mesh_id, mat_id, true);           
                    v3 c_pos = pos + v3(visual_direction_offsets[c]) - v3(0.5f);
                    m44 t = math::translate(c_pos) * math::scale(v3(0.07f));
                    r.transform = m44GPU(t);
                    
                    uint64 corner_id = hash_view(vtsw.tile_set->file_path) ^ hash_view(tile_entry.model_path) + c;
                    WidgetSystem::Mouse3DInfo mouse;
                    mouse.model = t;
                    mouse.mvp = scene->render_scene.viewport.camera->vp * mouse.model;
                    mouse.uv_position = scene->render_scene.viewport.mouse_uv();
                    mouse.os_ray = math::transformed_ray(mouse.mvp, mouse.uv_position);
                    v4 h_screen_position = mouse.mvp * v4(v3(0.0f), 1.0f);
                    v2 uv_screen_position = math::to_unsigned_range(h_screen_position.xy / h_screen_position.w);
                    mouse.os_center_ray = math::transformed_ray(mouse.mvp, uv_screen_position);
                    mouse.viewport_size = v2(scene->render_scene.viewport.size);

                    auto dot_project_info = mouse_to_3d_dot(mouse);

                    auto& depth = WidgetSystem::depths[corner_id];
                    if (dot_project_info.distance < 12.0f) {
                        constexpr float bias = 0.02f;
                        depth = math::min(depth, math::length(c_pos - scene->render_scene.viewport.camera->position) + bias);
                    } else {
                        depth = math::min(depth, FLT_MAX);
                    }
                    
                    if (WidgetSystem::pressed_id == corner_id) {
                        if (Input::alt)
                            tile_entry.corners[c] &= ~(Input::mouse_down[0] > 0.0f ? (0b1 << vtsw.setting) : 0);
                        else
                            tile_entry.corners[c] |= Input::mouse_down[0] > 0.0f ? (0b1 << vtsw.setting) : 0;
                    }
                }
            }
            
        }
    }
}

v3 to_vec(DirectionBits direction) {
    v3 v;
    v.x = direction & 4 ? 1.0f : -1.0f;
    v.y = direction & 2 ? 1.0f : -1.0f;
    v.z = direction & 1 ? 1.0f : -1.0f;
    return v;
}
DirectionBits to_direction_bits(v3 v) {
    uint32 value = NNN;
    if (v.x > 0)
        value += 4;
    if (v.y > 0)
        value += 2;
    if (v.z > 0)
        value += 1;
    return DirectionBits(value);
}

DirectionBits rotate_bits(uint32 direction, uint32 rotation) {
    return rotate_bits(DirectionBits(direction), rotation);
}
DirectionBits rotate_bits(DirectionBits direction, uint32 rotation) {
    if (rotation % 4 == 0)
        return direction;
    return to_direction_bits(math::rotate(to_vec(direction), rotation));
}

}