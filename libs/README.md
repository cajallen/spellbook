## Libraries
Why are they?

### EnTT
Needed for ECS structures for game-side logic. Using an ECS libraries allows for more cache-friendly data structures, and simpler iteration.

### fmt
Format is used for building strings. C++20 features would be used, but they do not have complete support yet.

### GLFW
GLFW is used for IO and windows. Replacing this requires a lot of OS interface code, and both win32 and X are ugly and bad.

### glm
glm is used as a fallback library for math functions. Not every single feature can be written by hand (though I wish it could be).

### imgui
Dear ImGui is used for the interface. Replacing this requires having a completed rendering API, which is a catch22.

### shaderc
Shaderc is used to compile libraries without syscalls. From research, it was the most convenient solution for doing this.

### stb
Sean Barrett's folder contains libraries used for image reading and writing currently. It is a dependency of tinygltf.

### tinygltf
tinygltf is a library used for loading gltf files, which feels mediocre to me. It relies on some libraries I don't want to rely on, and is a bit of a mess.

### tinyobj
tinyobj is a library for loading obj files. It's relatively simple, so it can stay for now.

### vk-bootstrap
vk-bootstrap is used for device and driver initialization. This is a one and done part of the process for Vulkan, so it is low priority for learning and for optimization. It was written by our good friend CharlesG (AKA Thomas Moletham)

### vma
Vulkan Memory Allocator is an AMD library for simplifying the VRAM alloc process into that more of an API rather than that of a procedure.