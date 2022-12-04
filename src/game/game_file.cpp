#include "game_file.hpp"

#include "general/logger.hpp"
#include "game/game.hpp"
#include "game/consumer.hpp"
#include "game/tower.hpp"
#include "game/tile.hpp"
#include "game/enemy.hpp"
#include "game/spawner.hpp"
#include "editor/console.hpp"
#include "renderer/assets/model.hpp"
#include "renderer/assets/mesh.hpp"
#include "renderer/assets/material.hpp"
#include "renderer/assets/texture.hpp"


namespace spellbook {

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
        case (FileType_Tower):
            return ".sbtow";
        case (FileType_Tile):
            return ".sbtil";
        case (FileType_Enemy):
            return ".sbenm";
        case (FileType_Spawner):
            return ".sbspw";
        case (FileType_Consumer):
            return ".sbcon";
    }
    log_warning("extension NYI");
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
         case (FileType_General):
         case (FileType_Model):
         case (FileType_Texture):
         case (FileType_Mesh):
         case (FileType_Material):
         case (FileType_Map):
         case (FileType_Tower):
         case (FileType_Tile):
         case (FileType_Enemy):
         case (FileType_Spawner):
         case (FileType_Consumer):
             return [type](const fs::path& path) { return path.extension().string() == extension(type); };
     }
     log_warning("extension NYI");
     return [](const fs::path& path) { return true; };
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
        case (FileType_Tower):
            return "DND_TOWER";
        case (FileType_Tile):
            return "DND_TILE";
        case (FileType_Enemy):
            return "DND_ENEMY";
        case (FileType_Spawner):
            return "DND_SPAWNER";
        case (FileType_Consumer):
            return "DND_CONSUMER";
    }
    log_warning("extension NYI");
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
    if (input == typeid(TowerPrefab))
        return FileType_Tower;
    if (input == typeid(TilePrefab))
        return FileType_Tile;
    if (input == typeid(EnemyPrefab))
        return FileType_Enemy;
    if (input == typeid(SpawnerPrefab))
        return FileType_Spawner;
    if (input == typeid(ConsumerPrefab))
        return FileType_Consumer;
    
    log_warning("extension NYI");
    return FileType_Unknown;
}

fs::path to_resource_path(const fs::path& path) {
    if (path.is_relative()) {
        if (path.string().starts_with(fs::path(game.resource_folder).lexically_proximate(fs::current_path()).string()))
            return (fs::current_path() / path).string();
        return (game.resource_folder / path).string();
    }
    else
        return path.string();
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

}