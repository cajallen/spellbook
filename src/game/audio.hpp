#pragma once

#include <miniaudio.h>
#include <plf_colony.h>

#include "general/math/geometry.hpp"
#include "general/file/file_path.hpp"

namespace spellbook {

struct Scene;

struct SoundSettings {
    bool global = false;
    v3 position = v3(0.0f);
    float volume = 1.0f;
};

struct Audio {
    ma_engine engine;
    ma_sound_group default_group;
    
    void setup();
    void update(Scene* scene);
    void shutdown();
    void play_sound(const FilePath& file_path, SoundSettings settings);

    plf::colony<ma_sound> sounds;

    void cleanup_done_sounds();
};


}
