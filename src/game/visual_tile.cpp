#include "visual_tile.hpp"

#include "extension/fmt.hpp"
#include "extension/imgui_extra.hpp"
#include "extension/icons/font_awesome4.h"
#include "general/logger.hpp"
#include "general/math.hpp"

namespace spellbook {

std::array<u8, 8> apply_rotation(std::array<u8, 8> corners, VisualTileRotation rotation) {
    if (rotation.flip) {
        std::swap(corners[NNN], corners[PNN]);
        std::swap(corners[NPN], corners[PPN]);
        std::swap(corners[NNP], corners[PNP]);
        std::swap(corners[NPP], corners[PPP]);
    }
    while (rotation.yaw > 0) {
        u8 backup_origin = corners[NNN];
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

bool get_rotation(std::array<u8, 8> corners, std::array<u8, 8> target, VisualTileRotation& out_rotation, u32 seed) {
    vector<VisualTileRotation> candidate_rotations = {};

    for (u8 i = 0; i <= 0b111; i++) {
        VisualTileRotation rotation {
            .yaw = u8(i & 0b11),
            .flip = bool(i & 0b100)
        };
        if (apply_rotation(corners, rotation) == target)
            candidate_rotations.push_back(rotation);
    }
    
    if (candidate_rotations.empty())
        return false;

    math::random_seed(seed);
    out_rotation = candidate_rotations[math::random_s32(candidate_rotations.size())];
    return true;
}

umap<v3i, VisualTileEntry> build_visual_tiles(const uset<v3i>& solids, const umap<std::array<u8, 8>, vector<string>>& entry_pool) {
    umap<v3i, VisualTileEntry> entries;

    for (const v3i& solid : solids) {
        for (int x = -1; x <= 1; ++x) {
            for (int y = -1; y <= 1; ++y) {
                for (int z = -1; z <= 1; ++z) {
                    entries[solid - v3i(x,y,z)] = {};
                }
            } 
        }
    }

    // TODO: add support for setting this seed
    // TODO: make the seeds local, rather than relying on the chaining.
    u32 seed = 0;
    for (auto& [pos, entry] : entries) {
        
        std::array<u8, 8> tile_corners = {};
        for (int i = 0; i < 8; ++i) {
            tile_corners[i] = solids.contains(pos + visual_direction_offsets[i]) ? 1 : 0;
        }

        vector<VisualTileEntry> candidate_entries = {};
        for (auto& [entry_corners, entry_model] : entry_pool) {
            VisualTileRotation entry_rotation;
            bool viable = get_rotation(entry_corners, tile_corners, entry_rotation, seed++);
            if (viable) {
                math::random_seed(seed++);
                candidate_entries.emplace_back(entry_model[math::random_s32(entry_model.size())], entry_rotation);
            }
        }
        if (candidate_entries.empty()) {
            message_queue.push({
                .str = fmt_("No candidates found for corners {} {} {} {} {} {} {} {}",
                    tile_corners[0], tile_corners[1], tile_corners[2], tile_corners[3],
                    tile_corners[4], tile_corners[5], tile_corners[6], tile_corners[7]),
                .color = palette::orange
            });
            continue;
        }
        math::random_seed(seed++);
        entry = candidate_entries[math::random_s32(candidate_entries.size())];
    }
    
    return entries;
}

umap<std::array<u8, 8>, vector<string>> convert_to_entry_pool(const VisualTileSet& tile_set) {
    umap<std::array<u8, 8>, vector<string>> entry_pool;

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
            ImGui::Dummy(ImVec2{40, 1});
            ImGui::SameLine();
            ImGui::InputScalar("NNP", ImGuiDataType_U8, &tile.corners[NNP], &step);
            ImGui::SameLine();
            ImGui::InputScalar("PNP", ImGuiDataType_U8, &tile.corners[PNP], &step);

            ImGui::InputScalar("NPN", ImGuiDataType_U8, &tile.corners[NPN], &step);
            ImGui::SameLine();
            ImGui::InputScalar("PPN", ImGuiDataType_U8, &tile.corners[PPN], &step);
            ImGui::SameLine();
            ImGui::Dummy(ImVec2{40, 1});
            ImGui::SameLine();
            ImGui::InputScalar("NPP", ImGuiDataType_U8, &tile.corners[NPP], &step);
            ImGui::SameLine();
            ImGui::InputScalar("PPP", ImGuiDataType_U8, &tile.corners[PPP], &step);
            ImGui::PopStyleVar();
            ImGui::PopItemWidth();

            changed |= ImGui::PathSelect("Model", &tile.model_path, "resources/models", FileType_Model, true);

            if (ImGui::Button("Rotate 1")) {
                tile.corners = apply_rotation(tile.corners, VisualTileRotation{.yaw = 1, .flip = false});
            }
            ImGui::SameLine();
            if (ImGui::Button("Flip")) {
                tile.corners = apply_rotation(tile.corners, VisualTileRotation{.yaw = 0, .flip = true});
            }
        }
        ImGui::Unindent();
        ImGui::PopID();
    }
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::Button(ICON_FA_PLUS)) {
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
                    tile_set->tiles.emplace_back(std::array<u8, 8>{}, dir_entry.path().string());
                }
            }
            
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    return changed;
}

}