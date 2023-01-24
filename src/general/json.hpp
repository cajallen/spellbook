#pragma once

#include <memory>
#include <variant>
#include <type_traits>
#include <magic_enum.hpp>

#include "vector.hpp"
#include "string.hpp"
#include "umap.hpp"

using std::get;
using std::istream;
using std::make_shared;
using std::shared_ptr;
using std::variant;

template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
};

template <class... Ts> overloaded(Ts ...) -> overloaded<Ts...>;

namespace spellbook {

template <typename J>
concept enum_concept = std::is_enum_v<J>;
template <typename J>
concept int_concept = std::is_integral_v<J>;
template <typename J>
concept float_concept = std::is_floating_point_v<J>;

struct json_value;
using json = umap<string, shared_ptr<json_value>>;

// JSON lib works by converting everything to and from this type
// Built-in types are declared here, User-types are declared there.
// JSON_IMPL used as a shortcut, where first arg is Type, and the rest are the members to be saved.
using json_variant = variant<json, vector<json_value>, string, bool, s64, f64>;

struct json_value {
    json_variant value;

    vector<json_value> get_list() const {
        return get<vector<json_value>>(value);
    }

    string dump() const;
};

json               parse(string& contents);
json               parse_file(const string& file_name);
json               parse_json(istream& iss);
json_value         parse_item(istream& iss);
vector<json_value> parse_list(istream& iss);
string             parse_quote(istream& iss);

void   file_dump(const json& json, const string& file_name);
string dump_json(const json& json);
void   delete_json(json& j);


template <typename JsonT> json_value                   to_jv(const vector<JsonT>& _vector);
template <typename JsonT> json_value                   to_jv(const vector<shared_ptr<JsonT>>& _vector);
template <typename JsonT1, typename JsonT2> json_value to_jv(umap<JsonT1, JsonT2> _map);
template <typename JsonT1, typename JsonT2> json_value to_jv(umap<shared_ptr<JsonT1>, JsonT2> _map);
template <typename JsonT> json_value                   to_jv(const umap<string, JsonT>& _map);
json_value                                      to_jv(vector<json_value> _vector);
json_value                                      to_jv(const json& input_json);
json_value                                      to_jv(const char* input_string);
json_value                                      to_jv(const string& input_string);
json_value                                      to_jv(bool input_bool);
template <int_concept T>
json_value to_jv(T input_int);
template <float_concept T>
json_value to_jv(T input_float);
template <enum_concept T>
json_value to_jv(T input_enum);


template <typename JsonT>
json_value to_jv(const vector<JsonT>& _vector) {
    vector<json_value> _list = {};
    for (const JsonT& e : _vector) {
        json_value jv  = to_jv(e);
        bool       add = true;
        visit(overloaded{
                [&](const json& j) {
                    if (j.size() == 0)
                        add = false;
                },
                [&](const vector<json_value>& jvs) {
                    if (jvs.size() == 0)
                        add = false;
                },
                [](auto a) {
                }
            },
            jv.value);
        if (add)
            _list.push_back(to_jv(e));
    }
    json_value jv;
    jv.value = json_variant{_list};
    return jv;
}

template <typename JsonT, size_t N>
json_value to_jv(const std::array<JsonT, N>& _array) {
    vector<json_value> _list = {};
    for (const JsonT& e : _array) {
        json_value jv  = to_jv(e);
        bool       add = true;
        visit(overloaded{
                [&](const json& j) {
                    if (j.size() == 0)
                        add = false;
                },
                [&](const vector<json_value>& jvs) {
                    if (jvs.size() == 0)
                        add = false;
                },
                [](auto a) {
                }
            },
            jv.value);
        if (add)
            _list.push_back(to_jv(e));
    }
    json_value jv;
    jv.value = json_variant{_list};
    return jv;
}

template <typename JsonT>
json_value to_jv(const uset<JsonT>& _vector) {
    vector<json_value> _list = {};
    for (const JsonT& e : _vector) {
        json_value jv  = to_jv(e);
        bool       add = true;
        visit(overloaded{
                [&](const json& j) {
                    if (j.size() == 0)
                        add = false;
                },
                [&](const vector<json_value>& jvs) {
                    if (jvs.size() == 0)
                        add = false;
                },
                [](auto a) {
                }
            },
            jv.value);
        if (add)
            _list.push_back(to_jv(e));
    }
    json_value jv;
    jv.value = json_variant{_list};
    return jv;
}

template <typename JsonT> json_value to_jv(const vector<shared_ptr<JsonT>>& _vector) {
    vector<json_value> _list = {};
    for (shared_ptr<JsonT> e : _vector)
        _list.push_back(to_jv(*e));
    json_value jv;
    jv.value = json_variant{_list};
    return jv;
}

template <typename JsonT1, typename JsonT2> json_value to_jv(umap<JsonT1, JsonT2> _map) {
    vector<json_value> _list = {};
    for (auto& [k, v] : _map) {
        json v_j{};
        v_j["key"]   = make_shared<json_value>(to_jv(k));
        v_j["value"] = make_shared<json_value>(to_jv(v));
        _list.push_back(to_jv(v_j));
    }
    json_value jv;
    jv.value = json_variant{_list};
    return jv;
}

template <typename JsonT1, typename JsonT2> json_value to_jv(umap<shared_ptr<JsonT1>, JsonT2> _map) {
    vector<json_value> _list = {};
    for (auto& [k, v] : _map) {
        json v_j{};
        v_j["key"]   = make_shared<json_value>(to_jv(*k));
        v_j["value"] = make_shared<json_value>(to_jv(v));
        _list.push_back(to_jv(v_j));
    }
    json_value jv;
    jv.value = json_variant{_list};
    return jv;
}

template <typename JsonT> json_value to_jv(const umap<string, JsonT>& _map) {
    json _json{};
    for (auto& [k, v] : _map)
        _json[k] = make_shared<json_value>(to_jv(v));
    json_value jv;
    jv.value = json_variant{_json};
    return jv;
}

json_value to_jv(vector<json_value> _vector);
json_value to_jv(const json& input_json);
json_value to_jv(const char* input_string);
json_value to_jv(const string& input_string);
json_value to_jv(bool input_bool);

template <float_concept T>
json_value to_jv(T input_float) {
    json_value jv;
    jv.value = json_variant{(f64) input_float};
    return jv;
}

template <int_concept T>
json_value to_jv(T input_int) {
    json_value jv;
    jv.value = json_variant{(s64) input_int};
    return jv;
}

template <enum_concept T>
json_value to_jv(T input_enum) {
    json_value jv;
    jv.value = json_variant{string(magic_enum::enum_name(input_enum))};
    return jv;
}


template <typename T>
T from_jv(const json_value& jv) {
    return from_jv_impl(jv, (T*) 0);
}

template <typename JsonT>
vector<JsonT> from_jv_impl(const json_value& jv, vector<JsonT>* _) {
    vector<JsonT>      t;
    vector<json_value> _list = jv.get_list();
    t.reserve(t.size() + _list.size());
    for (auto& e : _list) {
        t.push_back(from_jv<JsonT>(e));
    }
    return t;
}

template <typename JsonT, size_t N>
std::array<JsonT, N> from_jv_impl(const json_value& jv, std::array<JsonT, N>* _) {
    std::array<JsonT, N> t;
    vector<json_value> _list = jv.get_list();
    u32 i = 0;
    for (auto& e : _list) {
        t[i++] = from_jv<JsonT>(e);
    }
    return t;
}

template <typename JsonT>
uset<JsonT> from_jv_impl(const json_value& jv, uset<JsonT>* _) {
    uset<JsonT>      t;
    vector<json_value> _list = jv.get_list();
    for (auto& e : _list) {
        t.insert(from_jv<JsonT>(e));
    }
    return t;
}

template <typename JsonT>
vector<shared_ptr<JsonT>> from_jv_impl(const json_value& jv, vector<shared_ptr<JsonT>>* _) {
    vector<shared_ptr<JsonT>> t;
    vector<json_value>        _list = jv.get_list();
    t.reserve(t.size() + _list.size());
    for (auto& e : _list) {
        t.push_back(make_shared<JsonT>(from_jv<JsonT>(e)));
    }
    return t;
}

template <typename JsonS, typename JsonT>
umap<JsonS, JsonT> from_jv_impl(const json_value& jv, umap<JsonS, JsonT>* _) {
    umap<JsonS, JsonT> t;
    for (auto& value_json_value : jv.get_list()) {
        json  value_json = from_jv<json>(value_json_value);
        JsonS key        = from_jv<JsonS>(*value_json["key"]);
        JsonT value      = from_jv<JsonT>(*value_json["value"]);
        t[key]           = value;
    }
    return t;
}

template <typename JsonS, typename JsonT>
umap<shared_ptr<JsonS>, JsonT> from_jv_impl(const json_value& jv, umap<shared_ptr<JsonS>, JsonT>* _) {
    umap<shared_ptr<JsonS>, JsonT> t;
    for (auto& value_json_value : jv.get_list()) {
        json  value_json = from_jv<json>(value_json_value);
        JsonS key        = make_shared<JsonS>(from_jv<JsonS>(*value_json["key"]));
        JsonT value      = from_jv<JsonT>(*value_json["value"]);
        t[key]           = value;
    }
    return t;
}

template <typename JsonT>
umap<string, JsonT> from_jv_impl(const json_value& jv, umap<string, JsonT>* _) {
    umap<string, JsonT> t;
    for (auto& [k, val] : get<json>(jv.value)) {
        t[k] = from_jv<JsonT>(*val);
    }
    return t;
}

json   from_jv_impl(const json_value& jv, json* _);
bool   from_jv_impl(const json_value& jv, bool* _);
string from_jv_impl(const json_value& jv, string* _);

template <int_concept T>
T from_jv_impl(const json_value& jv, T* _) {
    return (T) get<s64>(jv.value);
}

template <float_concept T>
T from_jv_impl(const json_value& jv, T* _) {
    return (T) get<f64>(jv.value);
}

template <enum_concept T>
T from_jv_impl(const json_value& jv, T* _) {
    return magic_enum::enum_cast<T>(get<string>(jv.value)).value_or(T(0));
}

}

#define EXPAND(x) x
#define GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, NAME, ...) NAME
#define PASTE(...) \
    EXPAND(GET_MACRO(__VA_ARGS__, PASTE20, PASTE19, PASTE18, PASTE17, PASTE16, PASTE15, PASTE14, PASTE13, PASTE12, PASTE11, PASTE10, PASTE9, PASTE8, PASTE7, PASTE6, PASTE5, PASTE4, PASTE3, PASTE2, PASTE1)(__VA_ARGS__))
#define PASTE2(func, v1) func(v1)
#define PASTE3(func, v1, v2) PASTE2(func, v1) PASTE2(func, v2)
#define PASTE4(func, v1, v2, v3) PASTE2(func, v1) PASTE3(func, v2, v3)
#define PASTE5(func, v1, v2, v3, v4) PASTE2(func, v1) PASTE4(func, v2, v3, v4)
#define PASTE6(func, v1, v2, v3, v4, v5) PASTE2(func, v1) PASTE5(func, v2, v3, v4, v5)
#define PASTE7(func, v1, v2, v3, v4, v5, v6) PASTE2(func, v1) PASTE6(func, v2, v3, v4, v5, v6)
#define PASTE8(func, v1, v2, v3, v4, v5, v6, v7) PASTE2(func, v1) PASTE7(func, v2, v3, v4, v5, v6, v7)
#define PASTE9(func, v1, v2, v3, v4, v5, v6, v7, v8) PASTE2(func, v1) PASTE8(func, v2, v3, v4, v5, v6, v7, v8)
#define PASTE10(func, v1, v2, v3, v4, v5, v6, v7, v8, v9) PASTE2(func, v1) PASTE9(func, v2, v3, v4, v5, v6, v7, v8, v9)
#define PASTE11(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10) PASTE2(func, v1) PASTE10(func, v2, v3, v4, v5, v6, v7, v8, v9, v10)
#define PASTE12(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11) PASTE2(func, v1) PASTE11(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11)
#define PASTE13(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12) PASTE2(func, v1) PASTE12(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12)
#define PASTE14(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13) PASTE2(func, v1) PASTE13(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13)
#define PASTE15(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14) PASTE2(func, v1) PASTE14(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14)
#define PASTE16(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15) PASTE2(func, v1) PASTE15(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15)
#define PASTE17(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16) PASTE2(func, v1) PASTE16(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16)
#define PASTE18(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17) PASTE2(func, v1) PASTE17(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17)
#define PASTE19(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18) PASTE2(func, v1) PASTE18(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18)
#define PASTE20(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19) PASTE2(func, v1) PASTE19(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19)

#define FROM_JSON_ELE(var)                                     \
    if (j.contains(#var)) {                                    \
        value.var = from_jv<decltype(value.var)>(*j.at(#var)); \
    }
#define FROM_JSON_MEMBER(var)                      \
    if (j.contains(#var))                          \
        var = from_jv<decltype(var)>(*j.at(#var));

#define FROM_JSON_IMPL(Type, ...)                             \
    inline Type from_jv_impl(const json_value& jv, Type* _) { \
       json j = from_jv<json>(jv);                            \
       Type value;                                            \
       EXPAND(PASTE(FROM_JSON_ELE, __VA_ARGS__))              \
       return value;                                          \
    }

#define TO_JSON_ELE(var) \
        j[#var] = make_shared<json_value>(to_jv(value.var));

#define TO_JSON_MEMBER(var) \
    j[#var] = make_shared<json_value>(to_jv(var));

#define TO_JSON_IMPL(Type, ...)                  \
    inline json_value to_jv(const Type& value) { \
        auto j = json();                         \
        EXPAND(PASTE(TO_JSON_ELE, __VA_ARGS__))  \
        return to_jv(j);                         \
    }

#define JSON_IMPL(Type, ...)          \
    FROM_JSON_IMPL(Type, __VA_ARGS__) \
    TO_JSON_IMPL(Type, __VA_ARGS__)

#define JSON_IMPL_TEMPLATE(Template, Type, ...) \
Template FROM_JSON_IMPL(Type, __VA_ARGS__)      \
Template TO_JSON_IMPL(Type, __VA_ARGS__)
