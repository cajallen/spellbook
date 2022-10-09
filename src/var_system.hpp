#pragma once

#include "geometry.hpp"
#include "umap.hpp"
#include "string.hpp"

namespace spellbook {

struct VarBool {
    bool value;
    bool readonly = false;

    constexpr bool operator == (const VarBool& rhs) { return value == rhs.value && readonly == rhs.readonly; }
};

struct VarString {
    string value;
    bool readonly = false;

    constexpr bool operator == (const VarString& rhs) { return value == rhs.value && readonly == rhs.readonly; }
};

struct VarFloat {
    f32 value = {};
    f32 min = -FLT_MAX;
    f32 max =  FLT_MAX;
    f32 speed = 0.01f;
    bool readonly = false;

    constexpr bool operator == (const VarFloat& rhs) { return value == rhs.value && min == rhs.min && max == rhs.max && speed == rhs.speed && readonly == rhs.readonly; }
};

struct VarInt {
    int value = {};
    int min = -INT_MAX;
    int max = INT_MAX;
    f32 speed = 0.01f;
    bool readonly;

    constexpr bool operator == (const VarInt& rhs) { return value == rhs.value && min == rhs.min && max == rhs.max && speed == rhs.speed && readonly == rhs.readonly; }
};

struct VarV2 {
    v2 value = {};
    f32 min = -FLT_MAX;
    f32 max =  FLT_MAX;
    f32 speed = 0.01f;
    bool readonly = false;

    constexpr bool operator == (const VarV2& rhs) { return value == rhs.value && min == rhs.min && max == rhs.max && speed == rhs.speed && readonly == rhs.readonly; }
};

struct VarV3 {
    v3 value = {};
f32 min = -FLT_MAX;
    f32 max =  FLT_MAX;
    f32 speed = 0.01f;
    bool readonly = false;

    constexpr bool operator == (const VarV3& rhs) { return value == rhs.value && min == rhs.min && max == rhs.max && speed == rhs.speed && readonly == rhs.readonly; }
};

struct VarSystem {
    static umap<string, VarBool> bools;
    static umap<string, VarString> strings;
    static umap<string, VarInt> ints;
    static umap<string, VarFloat> floats;
    static umap<string, VarV2> v2s;
    static umap<string, VarV3> v3s;

    static VarBool& add_bool(string name, VarBool b);
    static VarString& add_string(string name, VarString s);
    static VarInt& add_int(string name, VarInt i);
    static VarFloat& add_float(string name, VarFloat f);
    static VarV2& add_v2(string name, VarV2 v);
    static VarV3& add_v3(string name, VarV3 v);
    static void window(bool* p_open);
};

}
