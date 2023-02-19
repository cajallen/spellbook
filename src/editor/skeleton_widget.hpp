#pragma once

// #include "renderer/assets/skeleton.hpp"

namespace spellbook {

struct m44;
struct RenderScene;
struct SkeletonCPU;
void skeleton_widget(SkeletonCPU* skeleton, const m44& model, RenderScene* render_scene);

}