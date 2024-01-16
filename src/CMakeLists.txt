add_subdirectory(archive)

add_library(spellbook_src
    h_impl.cpp
)
add_subdirectory(renderer)
add_subdirectory(editor)
add_subdirectory(game)

target_include_directories(spellbook_src PUBLIC .)
target_link_libraries(spellbook_src PUBLIC archive libs renderer)
target_precompile_headers(spellbook_src PUBLIC archive/general/global.hpp)
