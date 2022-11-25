#include "var_system.hpp"

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <tracy/Tracy.hpp>

#include "editor/console.hpp"

namespace spellbook {

umap<string, VarBool> VarSystem::bools;
umap<string, VarString> VarSystem::strings;
umap<string, VarInt> VarSystem::ints;
umap<string, VarFloat> VarSystem::floats;
umap<string, VarV2> VarSystem::v2s;
umap<string, VarV3> VarSystem::v3s;

VarBool& VarSystem::add_bool(string name, VarBool b) {
    if (bools.count(name) == 1) {
        if (b == bools[name]) 
            return bools[name];
        else
            console({.str = fmt_("{} initialized with contrasting values, overwriting", name), .group = "var_system", .color = palette::orange, .save = false});
    }
    bools[name] = b;
    return bools[name];
}

VarString& VarSystem::add_string(string name, VarString s) {
    if (strings.count(name) == 1) {
        if (s == strings[name]) 
            return strings[name];
        else
            console({.str = fmt_("{} initialized with contrasting values, overwriting", name), .group = "var_system", .color = palette::orange, .save = false});
    }
    strings[name] = s;
    return strings[name];
}

VarInt& VarSystem::add_int(string name, VarInt i) {
    if (ints.count(name) == 1) {
        if (i == ints[name]) 
            return ints[name];
        else
            console({.str = fmt_("{} initialized with contrasting values, overwriting", name), .group = "var_system", .color = palette::orange, .save = false});
    }
    ints[name] = i;
    return ints[name];
}

VarFloat& VarSystem::add_float(string name, VarFloat f) {
    if (floats.count(name) == 1) {
        if (f == floats[name]) 
            return floats[name];
        else
            console({.str = fmt_("{} initialized with contrasting values, overwriting", name), .group = "var_system", .color = palette::orange, .save = false});
    }
    floats[name] = f;
    return floats[name];
}

VarV2& VarSystem::add_v2(string name, VarV2 v) {
    if (v2s.count(name) == 1) {
        if (v == v2s[name]) 
            return v2s[name];
        else
            console({.str = fmt_("{} initialized with contrasting values, overwriting", name), .group = "var_system", .color = palette::orange, .save = false});
    }
    v2s[name] = v;
    return v2s[name];
}

VarV3& VarSystem::add_v3(string name, VarV3 v) {
    if (v3s.count(name) == 1) {
        if (v == v3s[name]) 
            return v3s[name];
        else
            console({.str = fmt_("{} initialized with contrasting values, overwriting", name), .group = "var_system", .color = palette::orange, .save = false});
    }
    v3s[name] = v;
    return v3s[name];
}
void VarSystem::window(bool* p_open) {
    ZoneScoped;
    if (ImGui::Begin("Var System", p_open)) {
        for (auto& [var_name, var] : bools) {
            ImGui::Checkbox(var_name.c_str(), &var.value);
        }
        for (auto& [var_name, var] : strings) {
            ImGui::InputText(var_name.c_str(), &var.value);
        }
        for (auto& [var_name, var] : ints) {
            ImGui::DragInt(var_name.c_str(), &var.value);
        }
        for (auto& [var_name, var] : floats) {
            ImGui::DragFloat(var_name.c_str(), &var.value);
        }
        for (auto& [var_name, var] : v2s) {
            ImGui::DragFloat2(var_name.c_str(), &var.value.x);
        }
        for (auto& [var_name, var] : v3s) {
            ImGui::DragFloat3(var_name.c_str(), &var.value.x);
        }
    }
    ImGui::End();
}

}
