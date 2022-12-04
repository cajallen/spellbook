cmake_minimum_required(VERSION 3.10)

project(spellbook)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
option(TRACY_ENABLE "" ON)
option(TRACY_ON_DEMAND "" ON)

add_subdirectory(src/general)
add_subdirectory(libs/imgui)
add_subdirectory(libs/vk-bootstrap)
add_subdirectory(libs/tinygltf)
add_subdirectory(libs/magic_enum)
add_subdirectory(libs/fmt)
add_subdirectory(libs/vuk)
add_subdirectory(libs/tracy)

set(source_files
	src/main.cpp
    src/h_impl.cpp
)
set(header_files
)

add_executable(spellbook source_fiels header_files)

target_link_libraries(spellbook 
    spellbook_general
    imgui
    vk-bootstrap::vk-bootstrap
    tinygltf::tinygltf
    magic_enum
    fmt
    vuk::vuk
    Tracy::TracyClient)

target_include_directories(spellbook PUBLIC
                           ${PROJECT_SOURCE_DIR}/src
                           ${PROJECT_SOURCE_DIR}/libs
                           )