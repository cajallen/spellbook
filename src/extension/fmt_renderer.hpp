#pragma once

#include <fmt/format.h>
#include <fmt/compile.h>

#include "extension/fmt_geometry.hpp"
#include "renderer/renderable.hpp"
#include "renderer/assets/model.hpp"


template <> struct fmt::formatter<spellbook::Renderable> : formatter<string> {
    template <typename FormatContext> auto format(const spellbook::Renderable& r, FormatContext& ctx) {
        auto out = ctx.out();

        out = fmt::format_to(out,
            "Renderable{{transform:{}",
            r.transform);
        return out;
    }
};

template <> struct fmt::formatter<spellbook::ModelCPU> : formatter<string> {
    template <typename FormatContext> auto format(const spellbook::ModelCPU& p, FormatContext& ctx) {
        auto out = ctx.out();
        
        return out;
    }
};
