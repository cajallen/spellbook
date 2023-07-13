#include "game_file.hpp"

#include "extension/imgui_extra.hpp"
#include "general/logger.hpp"
#include "renderer/assets/model.hpp"
#include "renderer/assets/mesh.hpp"
#include "renderer/assets/material.hpp"
#include "renderer/assets/texture.hpp"
#include "renderer/assets/particles.hpp"
#include "game/game.hpp"
#include "game/map.hpp"
#include "game/visual_tile.hpp"
#include "game/entities/consumer.hpp"
#include "game/entities/lizard.hpp"
#include "game/entities/tile.hpp"
#include "game/entities/enemy.hpp"
#include "game/entities/spawner.hpp"

namespace spellbook {

FileType file_type_from_path(const fs::path& path) {
    string extension_string = path.extension().string();
    for (uint32 i = 0; i < magic_enum::enum_count<FileType>(); i++) {
        FileType type = (FileType) i;
        if (extension(type) == extension_string)
            return type;
    }
    return FileType_Unknown;
}

string extension(FileType type) {
    switch (type) {
        case (FileType_General):
            return ".sbgen";
        case (FileType_Model):
            return ".sbmod";
        case (FileType_Texture):
            return ".sbtex";
        case (FileType_Mesh):
            return ".sbmsh";
        case (FileType_Material):
            return ".sbmat";
        case (FileType_Map):
            return ".sbmap";
        case (FileType_Lizard):
            return ".sbliz";
        case (FileType_Tile):
            return ".sbtil";
        case (FileType_Enemy):
            return ".sbenm";
        case (FileType_Spawner):
            return ".sbspw";
        case (FileType_Consumer):
            return ".sbcon";
        case (FileType_Emitter):
            return ".sbemt";
        case (FileType_VisualTileSet):
            return ".sbvts";
        case (FileType_Drop):
            return ".sbdrp";
        case (FileType_Skeleton):
            return ".sbskl";
    }
    return "NYI";
}

std::function<bool(const fs::path&)> path_filter(FileType type) {
     switch (type) {
         case (FileType_Directory):
             return [](const fs::path& path) { return is_directory(path); };
         case (FileType_Unknown):
             return [](const fs::path& path) { return is_regular_file(path); };
         case (FileType_ModelAsset):
             return [](const fs::path& path) { return vector<string>{".gltf", ".glb"}.contains(path.extension().string()); };
         case (FileType_TextureAsset):
             return [](const fs::path& path) { return vector<string>{".png", ".jpg", ".jpeg"}.contains(path.extension().string()); };
         default:
             return [type](const fs::path& path) { return path.extension().string() == extension(type); };
     }
}

string dnd_key(FileType type) {
    switch (type) {
        case (FileType_Directory):
            return "DND_DIRECTORY";
        case (FileType_Unknown):
            return "DND_UNKNOWN";
        case (FileType_General):
            return "DND_GENERAL";
        case (FileType_Model):
            return "DND_MODEL";
        case (FileType_ModelAsset):
            return "DND_MODEL_ASSET";
        case (FileType_Texture):
            return "DND_TEXTURE";
        case (FileType_TextureAsset):
            return "DND_TEXTURE_ASSET";
        case (FileType_Mesh):
            return "DND_MESH";
        case (FileType_Material):
            return "DND_MATERIAL";
        case (FileType_Map):
            return "DND_MAP";
        case (FileType_Lizard):
            return "DND_LIZARD";
        case (FileType_Tile):
            return "DND_TILE";
        case (FileType_Enemy):
            return "DND_ENEMY";
        case (FileType_Spawner):
            return "DND_SPAWNER";
        case (FileType_Consumer):
            return "DND_CONSUMER";
        case (FileType_Emitter):
            return "DND_EMITTER";
        case (FileType_VisualTileSet):
            return "DND_VISUAL_TILE_SET";
        case (FileType_Drop):
            return "DND_DROP";
        case (FileType_Skeleton):
            return "DND_SKELETON";    }
    log_error("extension NYI");
    return "DND_UNKNOWN";
}

FileType from_typeinfo(const type_info& input) {
    if (input == typeid(ModelCPU))
        return FileType_Model;
    if (input == typeid(TextureCPU))
        return FileType_Texture;
    if (input == typeid(MeshCPU))
        return FileType_Mesh;
    if (input == typeid(MaterialCPU))
        return FileType_Material;
    if (input == typeid(LizardPrefab))
        return FileType_Lizard;
    if (input == typeid(TilePrefab))
        return FileType_Tile;
    if (input == typeid(EnemyPrefab))
        return FileType_Enemy;
    if (input == typeid(SpawnerPrefab))
        return FileType_Spawner;
    if (input == typeid(ConsumerPrefab))
        return FileType_Consumer;
    if (input == typeid(MapPrefab))
        return FileType_Map;
    if (input == typeid(EmitterCPU))
        return FileType_Emitter;
    if (input == typeid(VisualTileSet))
        return FileType_VisualTileSet;
    if (input == typeid(BeadPrefab))
        return FileType_Drop;
    if (input == typeid(SkeletonPrefab))
        return FileType_Skeleton;
    
    log_error("extension NYI");
    return FileType_Unknown;
}

FileCategory file_category(FileType type) {
    switch (type) {
        case (FileType_Texture):
        case (FileType_Mesh):
            return FileCategory_Asset;
        case (FileType_General):
        case (FileType_Map):
        case (FileType_Model):
        case (FileType_Skeleton):
        case (FileType_Material):
        case (FileType_Lizard):
        case (FileType_Tile):
        case (FileType_Enemy):
        case (FileType_Spawner):
        case (FileType_Consumer):
        case (FileType_Emitter):
        case (FileType_VisualTileSet):
        case (FileType_Drop):
            return FileCategory_Json;
        default:
            return FileCategory_Other;
    }
}

fs::path to_resource_path(const fs::path& path) {
    fs::path ret;
    if (path.is_relative()) {
        if (path.string().starts_with(fs::path(game.resource_folder).lexically_proximate(fs::current_path()).string()))
            return (fs::current_path() / path).string();
        return (game.resource_folder / path).string();
    }
    return path;
}

fs::path from_resource_path(const fs::path& path) {
    if (path.is_relative()) {
        if (path.string().starts_with(fs::path(game.resource_folder).lexically_proximate(fs::current_path()).string()))
            return path.lexically_proximate(fs::path(game.resource_folder).lexically_proximate(fs::current_path()));
        return path;
    }
    else {
        return path.lexically_proximate(game.resource_folder);
    }
}

bool inspect_dependencies(vector<string>& dependencies, const string& current_path) {
    bool changed = false;
    
    if (ImGui::TreeNode("Dependencies")) {
        if (ImGui::Button("Auto Populate")) {
            string contents = get_contents(to_resource_path(current_path).string());

            uint64 start_at = 0;
            while (start_at < contents.size()) {
                uint64 sb_index = contents.find(".sb", start_at);
                if (sb_index == string::npos)
                    break;
                uint64 start_quote = contents.rfind('"', sb_index) + 1;
                uint64 end_quote = contents.find_first_of('"', sb_index);
                dependencies.push_back(string(contents.data() + start_quote, end_quote - start_quote));
                
                start_at = end_quote;
            }
            
        }
        changed |= ImGui::UnorderedVector(dependencies,
            [](string& dep) {
                float width = ImGui::GetContentRegionAvail().x;
                float text_width = ImGui::CalcTextSize("Path").x;
                ImGui::SetNextItemWidth(width - 8.f - text_width);
                return ImGui::PathSelect("Path", &dep, "resources", FileType_Unknown);
            },
            [](vector<string>& deps, bool pressed) {
                if (pressed) {
                    deps.emplace_back();
                }
            },
            true);
        ImGui::TreePop();
    }
    return changed;
}


}

namespace ImGui {

bool PathSelect(const string& hint, fs::path* out, const fs::path& base_folder, spellbook::FileType type, int open_subdirectories, const std::function<void(const fs::path&)>& context_callback) {
    return PathSelect(hint, out, base_folder, path_filter(type), dnd_key(type), open_subdirectories, context_callback);
}
bool PathSelect(const string& hint, string* out, const string& base_folder, spellbook::FileType type, int open_subdirectories, const std::function<void(const fs::path&)>& context_callback) {
    return PathSelect(hint, out, base_folder, path_filter(type), dnd_key(type), open_subdirectories, context_callback);
}

}

