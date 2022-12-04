add_library(spellbook_general)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

target_sources(spellbook_general PRIVATE 
	astar.cpp
    color.cpp
    file.cpp
    geometry.cpp
    json.cpp
    logger.cpp
    math.cpp
    matrix_math.cpp
    matrix.cpp
    quaternion.cpp
    var_system.cpp)