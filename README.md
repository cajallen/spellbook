# Tower Defense Game
A currently unnamed tower defense game, inspired by Super Auto Pets and SNKRX.

# Spellbook
The engine, built in parallel and from relative scratch, that the game runs on. There is very minimal isolation between the game and the engine (and the editor for that matter), so this is more of a concept than a real thing. From these reasons comes this as a working name.

## Project goals
The primary goal of this project is to avoid theoretical gains, and focus on making tangible progress. 


## Temp Build Instructions
1. Initialize submodules
2. Rename libs/fmt/src/format.cc to libs/fmt_src/fmt_format.cc
3. If step 2 upsets you, tell me how to avoid the incremental compilation error that results from not doing that
4. Fix Vulkan include header paths and paths for linker
5. If step 4 upsets you, give me a working CMake file 