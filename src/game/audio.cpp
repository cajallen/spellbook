#include "audio.hpp"

#include "extension/fmt.hpp"
#include "general/logger.hpp"
#include "game/game_file.hpp"
#include "game/scene.hpp"
#include "editor/console.hpp"

namespace spellbook {

void Audio::setup() {
    ma_result result;
    result = ma_engine_init(nullptr, &engine);
    if (result != MA_SUCCESS)
        log_warning("audio init failed");

    result = ma_sound_group_init(&engine, 0, nullptr, &default_group);
    if (result != MA_SUCCESS)
        log_warning("group init failed");

    ma_sound_group_set_volume(&default_group, 1.0f);
    ma_engine_listener_set_world_up(&engine, 0, 0.0f, 0.0f, 1.0f);
}

void Audio::update(Scene* scene) {
    cleanup_done_sounds();
    
    const v3& pos = scene->camera.position;
    v3 vec = math::euler2vector(scene->camera.heading);
    ma_engine_listener_set_position(&engine, 0, pos.x, pos.y, pos.z);
    ma_engine_listener_set_direction(&engine, 0, vec.x, vec.y, vec.z);
}


void Audio::shutdown() {
    for (auto& sound : sounds)
        ma_sound_uninit(&sound);
    
    ma_engine_uninit(&engine);
}



void Audio::play_sound(const string& file_path, SoundSettings settings) {
    ma_sound& sound = *sounds.emplace();
    ma_uint32 flags = 0;
    if (settings.global)
        flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;
    ma_result result = ma_sound_init_from_file(&engine, to_resource_path(file_path).string().c_str(), flags, nullptr, nullptr, &sound);
    if (result != MA_SUCCESS) {
        console({.str=fmt_("Playing sound '{}' failed", file_path), .group="audio", .color = palette::orange});
        return;
    }
    ma_sound_set_position(&sound, settings.position.x, settings.position.y, settings.position.z);
    ma_sound_set_volume(&sound, settings.volume);
    ma_sound_start(&sound);
}

void Audio::cleanup_done_sounds() {
    for (auto it = sounds.begin(); it != sounds.end();) {
        if (it->atEnd) {
            ma_sound_uninit(&*it);
            it = sounds.erase(it);
        }
        else
            it++;
    }
}


}
