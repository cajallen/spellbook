#pragma once

#include <fmt/format.h>
#include <fmt/compile.h>

#include "renderer/renderable.hpp"
#include "renderer/render_scene.hpp"

#include <string>
using std::string;

template <> struct fmt::formatter<Renderable> : formatter<string> {
    template <typename FormatContext> auto format(const Renderable& r, FormatContext& ctx) {
        auto out = ctx.out();

        if (r.scene) {
            out = fmt::format_to(out,
                "Renderable{{name:\"{}\", mesh:\"{}\", material:\"{}\", scene:\"{}\", parent:{}}}",
                r.name,
                r.mesh,
                r.material,
                r.scene->name,
                r.scene->renderables.valid(r.parent) ? "\"" + r.scene->renderables[r.parent].name + "\"" : "None");
        } else {
            out = fmt::format_to(out, "Renderable{{name:\"{}\", mesh:\"{}\", material:\"{}\"}}", r.name, r.mesh, r.material);
        }
        return out;
    }
};

template <> struct fmt::formatter<Prefab> : formatter<string> {
    template <typename FormatContext> auto format(const Prefab& p, FormatContext& ctx) {
        auto out = ctx.out();

        out = fmt::format_to(out, "Prefab{{renderables:{{");
        for (int i = 0; i < p.renderables.size(); i++) {
            if (i != 0)
                out = fmt::format_to(out, ", ");
            out = fmt::format_to(out, "{}", p.renderables[i]);
        }
        out = fmt::format_to(out, "}}, root_nodes: [");
        for (int i = 0; i < p.root_nodes.size(); i++) {
            if (i != 0)
                out = fmt::format_to(out, ", ");
            out = fmt::format_to(out, "{}", p.root_nodes[i]);
        }
        out       = fmt::format_to(out, "], hierarchy: [");
        bool init = false;
        for (auto [r_i, p_i] : p.parents) {
            if (init)
                out = fmt::format_to(out, ", ");
            out  = fmt::format_to(out, "(c:{}, p:{})", r_i, p_i);
            init = true;
        }
        out = fmt::format_to(out, "]}}");
        return out;
    }
};
