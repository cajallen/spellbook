#include "mesh.hpp"

#include <imgui.h>

#include "console.hpp"

#include "math.hpp"

namespace spellbook {

void inspect(MeshGPU* mesh) {
    ImGui::Text(fmt_("Vertex Count: {}, Index Count: {}", mesh->vertex_count, mesh->index_count).c_str());
}

static v3 get_tangent(Vertex a, Vertex b, Vertex c) {
    v3 ab    = b.position - a.position;
    v3 ac    = c.position - a.position;
    v2 ab_uv = b.uv - a.uv;
    v2 ac_uv = c.uv - a.uv;
    if (ab_uv == v2(0) || ac_uv == v2(0) || ab_uv == ac_uv) {
        ab_uv = v2(1, 0);
        ac_uv = v2(0, 1);
    }
    auto f            = 1.0f / math::cross(ab_uv, ac_uv);
    v3   unnormalized = f * (ac_uv.y * ab - ab_uv.y * ac);
    if (unnormalized == v3(0))
        unnormalized = v3(1, 0, 0);
    return math::normalize(unnormalized);
}


}
