#include "game_path.hpp"

#include "game/game.hpp"

namespace spellbook {

FilePath resource_path(const string& val) {
    return FilePath(string(game.resource_folder) + val);
}

FilePath operator""_rp(const char* str, uint64 length) {
    return resource_path(string(str, length));
}

}