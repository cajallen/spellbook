# subdirectories
add_subdirectory(entt)
add_subdirectory(magic_enum)
add_subdirectory(tinygltf)
add_subdirectory(vuk)
add_subdirectory(tracy)
add_subdirectory(vk-bootstrap)

# fetches
include(FetchContent)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw
    GIT_TAG master
)
FetchContent_MakeAvailable(glfw)

# manual libs
add_library(imgui
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp
    imgui/backends/imgui_impl_glfw.cpp
    imgui/misc/cpp/imgui_stdlib.cpp
)
target_include_directories(imgui PUBLIC imgui imgui/backends)
target_link_libraries(imgui PRIVATE glfw)

add_library(lz4 lz4/lz4.cpp lz4/lz4hc.cpp lz4/xxhash.cpp)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE vuk/ext/stb)

add_library(miniaudio INTERFACE)
target_include_directories(miniaudio INTERFACE miniaudio)

# package as interface
add_library(libs INTERFACE)
target_link_libraries(libs INTERFACE 
    magic_enum
    tinygltf
    vuk
    glfw
    lz4
    vk-bootstrap
    imgui
    vk-bootstrap
    robin_hood
    fmt
    miniaudio
    stb
)
target_include_directories(libs INTERFACE 
    tracy/public
    entt/src
)

set(FA_PATH "fa.ttf" CACHE PATH "Font Awesome Path")

target_include_directories(libs INTERFACE .)
target_compile_definitions(libs INTERFACE
    IMGUI_USER_CONFIG=\"imgui_config.h\"
    FONT_ICON_FILE_NAME_FA=\"${FA_PATH}\"
)