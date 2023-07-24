cmake_minimum_required(VERSION 3.16)

project(spellbook)

set(CMAKE_CXX_STANDARD 20)

set(RESOURCE_PARENT_DIR ${CMAKE_SOURCE_DIR} CACHE PATH "Root directory for resources")
set(FA_PATH "${CMAKE_SOURCE_DIR}/external_resources/fonts/fa.ttf" CACHE PATH "Font Awesome path" FORCE)

add_subdirectory(libs)
add_subdirectory(src)

add_executable(spellbook_editor src/main.cpp)

target_compile_definitions(spellbook_editor PUBLIC 
	STBI_MSC_SECURE_CRT
	RESOURCE_PARENT_DIR=\"${RESOURCE_PARENT_DIR}\"
)

target_link_libraries(spellbook_editor PUBLIC libs)
target_link_libraries(spellbook_editor PUBLIC spellbook_src)
