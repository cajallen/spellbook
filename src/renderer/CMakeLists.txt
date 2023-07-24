find_package(Vulkan REQUIRED)

target_sources(spellbook_src PUBLIC
	assets/asset_loader.cpp
    assets/material.cpp
    assets/mesh_asset.cpp
    assets/mesh.cpp
    assets/model.cpp
    assets/particles.cpp
    assets/skeleton.cpp
    assets/texture_asset.cpp
    assets/texture.cpp
    camera.cpp
    draw_functions.cpp
    render_scene.cpp
    renderable.cpp
    renderer.cpp
    samplers.cpp
    vertex.cpp
    viewport.cpp
)