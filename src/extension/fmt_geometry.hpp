#pragma once

#include <fmt/format.h>
#include <fmt/compile.h>

#include "lib/color.hpp"
#include "lib/geometry.hpp"
#include "lib/quaternion.hpp"
#include "lib/matrix.hpp"

template <> struct fmt::formatter<spellbook::v2> : formatter<f32> {
    template <typename FormatContext> auto format(const spellbook::v2& vec, FormatContext& ctx) {
        auto out = ctx.out();
        *out     = '(';
        ctx.advance_to(out);
        out = formatter<f32>::format(vec.x, ctx);
        out = fmt::format_to(out, ", ");
        ctx.advance_to(out);
        out  = formatter<f32>::format(vec.y, ctx);
        *out = ')';
        return out;
    }
};

template <> struct fmt::formatter<spellbook::v2i> : formatter<s32> {
    template <typename FormatContext> auto format(const spellbook::v2i& vec, FormatContext& ctx) {
        auto out = ctx.out();
        *out     = '(';
        ctx.advance_to(out);
        out = formatter<s32>::format(vec.x, ctx);
        out = fmt::format_to(out, ", ");
        ctx.advance_to(out);
        out  = formatter<s32>::format(vec.y, ctx);
        *out = ')';
        return out;
    }
};

template <> struct fmt::formatter<spellbook::euler> : formatter<f32> {
    template <typename FormatContext> auto format(const spellbook::euler& eul, FormatContext& ctx) {
        auto out = ctx.out();
        *out     = '(';
        out      = fmt::format_to(out, "yaw:");
        ctx.advance_to(out);
        out = formatter<f32>::format(eul.yaw, ctx);
        out = fmt::format_to(out, ", pitch:");
        ctx.advance_to(out);
        out  = formatter<f32>::format(eul.pitch, ctx);
        *out = ')';
        return out;
    }
};

template <> struct fmt::formatter<spellbook::v3> : formatter<f32> {
    template <typename FormatContext> auto format(const spellbook::v3& vec, FormatContext& ctx) {
        auto out = ctx.out();
        *out     = '(';
        ctx.advance_to(out);
        out = formatter<f32>::format(vec.x, ctx);
        out = fmt::format_to(out, ", ");
        ctx.advance_to(out);
        out = formatter<f32>::format(vec.y, ctx);
        out = fmt::format_to(out, ", ");
        ctx.advance_to(out);
        out  = formatter<f32>::format(vec.z, ctx);
        *out = ')';
        return out;
    }
};

template <> struct fmt::formatter<spellbook::v3i> : formatter<s32> {
    template <typename FormatContext> auto format(const spellbook::v3i& vec, FormatContext& ctx) {
        auto out = ctx.out();
        *out     = '(';
        ctx.advance_to(out);
        out = formatter<s32>::format(vec.x, ctx);
        out = fmt::format_to(out, ", ");
        ctx.advance_to(out);
        out = formatter<s32>::format(vec.y, ctx);
        out = fmt::format_to(out, ", ");
        ctx.advance_to(out);
        out  = formatter<s32>::format(vec.z, ctx);
        *out = ')';
        return out;
    }
};

template <> struct fmt::formatter<spellbook::v4> : formatter<f32> {
    template <typename FormatContext> auto format(const spellbook::v4& vec, FormatContext& ctx) {
        auto out = ctx.out();
        *out     = '(';
        ctx.advance_to(out);
        out = formatter<f32>::format(vec.x, ctx);
        out = fmt::format_to(out, ", ");
        ctx.advance_to(out);
        out = formatter<f32>::format(vec.y, ctx);
        out = fmt::format_to(out, ", ");
        ctx.advance_to(out);
        out = formatter<f32>::format(vec.z, ctx);
        out = fmt::format_to(out, ", ");
        ctx.advance_to(out);
        out  = formatter<f32>::format(vec.w, ctx);
        *out = ')';
        return out;
    }
};

template <> struct fmt::formatter<spellbook::l2> : formatter<f32> {
    template <typename FormatContext> auto format(const spellbook::l2& line, FormatContext& ctx) {
        auto out = ctx.out();
        *out     = '(';
        out      = fmt::format_to(out, "start:");
        ctx.advance_to(out);
        out = formatter<f32>::format(line.start, ctx);
        out = fmt::format_to(out, ", end:");
        ctx.advance_to(out);
        out  = formatter<f32>::format(line.end, ctx);
        *out = ')';
        return out;
    }
};

template <> struct fmt::formatter<spellbook::r3> : formatter<f32> {
    template <typename FormatContext> auto format(const spellbook::r3& ray, FormatContext& ctx) {
        auto out = ctx.out();
        *out     = '(';
        out      = fmt::format_to(out, "origin:");
        ctx.advance_to(out);
        out = formatter<f32>::format(ray.origin, ctx);
        out = fmt::format_to(out, ", dir:");
        ctx.advance_to(out);
        out  = formatter<f32>::format(ray.dir, ctx);
        *out = ')';
        return out;
    }
};

template <> struct fmt::formatter<spellbook::quat> : formatter<f32> {
    template <typename FormatContext> auto format(const spellbook::quat& q, FormatContext& ctx) {
        auto out = ctx.out();
        *out     = '(';
        ctx.advance_to(out);
        out = formatter<f32>::format(q.x, ctx);
        out = fmt::format_to(out, ", ");
        ctx.advance_to(out);
        out = formatter<f32>::format(q.y, ctx);
        out = fmt::format_to(out, ", ");
        ctx.advance_to(out);
        out = formatter<f32>::format(q.z, ctx);
        out = fmt::format_to(out, ", ");
        ctx.advance_to(out);
        out  = formatter<f32>::format(q.w, ctx);
        *out = ')';
        return out;
    }
};

template <> struct fmt::formatter<spellbook::m33> : formatter<f32> {
    template <typename FormatContext> auto format(const spellbook::m33& m, FormatContext& ctx) {
        auto out = ctx.out();
        *out     = '[';
        for (int y = 0; y < 3; y++) {
            ctx.advance_to(out);
            out = fmt::format_to(out, "[");
            for (int x = 0; x < 3; x++) {
                ctx.advance_to(out);
                out = formatter<f32>::format(m.cr(x, y), ctx);
                if (x + 1 != 3)
                    out = fmt::format_to(out, ", ");
            }
            ctx.advance_to(out);
            out = fmt::format_to(out, "]\n");
        }
        *out = ']';
        return out;
    }
};

template <> struct fmt::formatter<spellbook::m44> : formatter<f32> {
    template <typename FormatContext> auto format(const spellbook::m44& m, FormatContext& ctx) {
        auto out = ctx.out();
        *out     = '[';
        for (int y = 0; y < 4; y++) {
            ctx.advance_to(out);
            out = fmt::format_to(out, "[");
            for (int x = 0; x < 4; x++) {
                ctx.advance_to(out);
                out = formatter<f32>::format(m.cr(x, y), ctx);
                if (x + 1 != 4)
                    out = fmt::format_to(out, ", ");
            }
            ctx.advance_to(out);
            out = fmt::format_to(out, "]");
            if (y + 1 != 4) {
                ctx.advance_to(out);
                out = fmt::format_to(out, "\n");
            }
        }
        *out = ']';
        return out;
    }
};

template <> struct fmt::formatter<spellbook::Color> : formatter<f32> {
    template <typename FormatContext> auto format(const spellbook::Color& col, FormatContext& ctx) {
        auto out = ctx.out();
        *out     = '(';
        ctx.advance_to(out);
        out = formatter<f32>::format(col.r, ctx);
        out = fmt::format_to(out, ", ");
        ctx.advance_to(out);
        out = formatter<f32>::format(col.g, ctx);
        out = fmt::format_to(out, ", ");
        ctx.advance_to(out);
        out = formatter<f32>::format(col.b, ctx);
        out = fmt::format_to(out, ", ");
        ctx.advance_to(out);
        out  = formatter<f32>::format(col.a, ctx);
        *out = ')';
        return out;
    }
};