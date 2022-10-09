#pragma once

#include "renderer.hpp"
#include "renderable.hpp"
#include "prefab.hpp"
#include "texture.hpp"
#include "mesh.hpp"
#include "material.hpp"

#include "vector.hpp"
#include "string.hpp"

TextureCPU load_texture(Renderer& renderer, const string& asset_path);
MeshCPU load_mesh(Renderer& renderer, const string& asset_path);
MaterialCPU load_material(Renderer& renderer, const string& asset_path);
