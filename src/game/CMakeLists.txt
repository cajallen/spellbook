add_subdirectory(entities)
add_subdirectory(effects)
add_subdirectory(gui)
target_sources(spellbook_src PUBLIC
    audio.cpp
    camera_controller.cpp
    game.cpp
    map_targeting.cpp
    map.cpp
    player.cpp
    pose_controller.cpp
    scene.cpp
    shop.cpp
    systems.cpp
    tile_set_generator.cpp
    timer.cpp
    visual_tile.cpp
)