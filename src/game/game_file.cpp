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

FileType get_type(const FilePath& path) {
    string extension_string = path.rel_path().extension().string();
    for (uint32 i = 0; i < magic_enum::enum_count<FileType>(); i++) {
        FileType type = (FileType) i;
        if (path.extension() == extension_string)
            return type;
    }
    return FileType_Unknown;
}
FileCategory get_category(const FilePath& path) {
    return file_category(get_type(path));
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
            return "DND_SKELETON";
    }
    log_error("extension NYI");
    return "DND_UNKNOWN";
}

string resource_folder(FileType type) {
    switch (type) {
        case FileType_Consumer:
            return FilePath(string(game.resource_folder) + "consumers").abs_string();
        case FileType_Drop:
            return FilePath(string(game.resource_folder) + "drops").abs_string();
        case FileType_Emitter:
            return FilePath(string(game.resource_folder) + "emitters").abs_string();
        case FileType_Enemy:
            return FilePath(string(game.resource_folder) + "enemies").abs_string();
        case FileType_Lizard:
            return FilePath(string(game.resource_folder) + "lizards").abs_string();
        case FileType_Map:
            return FilePath(string(game.resource_folder) + "maps").abs_string();
        case FileType_Model:
        case FileType_Skeleton:
            return FilePath(string(game.resource_folder) + "models").abs_string();
        case FileType_Spawner:
            return FilePath(string(game.resource_folder) + "spawners").abs_string();
        case FileType_Tile:
            return FilePath(string(game.resource_folder) + "tiles").abs_string();
        case FileType_VisualTileSet:
            return FilePath(string(game.resource_folder) + "visual_tile_sets").abs_string();
        default:
            return string(game.resource_folder);
    }
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

bool inspect_dependencies(vector<FilePath>& dependencies, const FilePath& current_path) {
    bool changed = false;
    
    if (ImGui::TreeNode("Dependencies")) {
        if (ImGui::Button("Auto Populate")) {
            string contents = get_contents(current_path);

            uint64 start_at = 0;
            while (start_at < contents.size()) {
                uint64 sb_index = contents.find(".sb", start_at);
                if (sb_index == string::npos)
                    break;
                uint64 start_quote = contents.rfind('"', sb_index) + 1;
                uint64 end_quote = contents.find_first_of('"', sb_index);
                string quote_contents = string(contents.data() + start_quote, end_quote - start_quote);
                dependencies.push_back(FilePath(quote_contents));
                
                start_at = end_quote;
            }
            
        }
        changed |= ImGui::UnorderedVector(dependencies,
            [](FilePath& dep) {
                float width = ImGui::GetContentRegionAvail().x;
                float text_width = ImGui::CalcTextSize("Path").x;
                ImGui::SetNextItemWidth(width - 8.f - text_width);
                return ImGui::PathSelect("Path", &dep, FileType_Unknown);
            },
            [](vector<FilePath>& deps, bool pressed) {
                if (pressed) {
                    deps.emplace_back();
                }
            },
            true);
        ImGui::TreePop();
    }
    return changed;
}

string get_contents(const FilePath& file_name, bool binary) {
    string path = file_name.abs_string();
    FILE* f = fopen(path.c_str(), !binary ? "r" : "rb");
    if (f == nullptr)
        return {};

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f) / sizeof(char);
    string contents;
    contents.resize(size);
    rewind(f);

    size_t read_bytes = fread(contents.data(), sizeof(char), size, f);

    fclose(f);

    contents.resize(std::min(strlen(contents.data()), read_bytes));
    return contents;
}

vector<uint32> get_contents_uint32(const FilePath& file_name, bool binary) {
    string path = file_name.abs_string();
    FILE* f = fopen(path.c_str(), !binary ? "r" : "rb");
    if (f == nullptr)
        return {};

    fseek(f, 0, SEEK_END);
    size_t    size = ftell(f) / sizeof(uint32);
    vector<uint32> contents;
    contents.resize(size);
    rewind(f);

    fread(&contents[0], sizeof(uint32), size, f);

    fclose(f);

    return contents;
}


}

namespace ImGui {

bool PathSelect(const string& hint, spellbook::FilePath* out, const spellbook::FilePath& base_folder, spellbook::FileType type, int open_subdirectories, const std::function<void(const fs::path&)>& context_callback) {
    fs::path out_path = out->abs_path();
    bool changed = PathSelect(hint, &out_path, base_folder.abs_string(), path_filter(type), dnd_key(type), open_subdirectories, context_callback);
    if (changed) {
        *out = spellbook::FilePath(out_path);
        return true;
    }
    return false;
}
bool PathSelect(const string& hint, spellbook::FilePath* out, spellbook::FileType type, int open_subdirectories, const std::function<void(const fs::path&)>& context_callback) {
    fs::path out_path = out->abs_path();
    bool changed = PathSelect(hint, &out_path, resource_folder(type), path_filter(type), dnd_key(type), open_subdirectories, context_callback);
    if (changed) {
        *out = spellbook::FilePath(out_path);
        return true;
    }
    return false;

}

}

