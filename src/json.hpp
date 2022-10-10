#pragma once

#include <memory>
#include <variant>
#include <type_traits>

#include "vector.hpp"
#include "string.hpp"
#include "umap.hpp"

using std::get;
using std::istream;
using std::make_shared;
using std::shared_ptr;
using std::variant;

template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

namespace spellbook {

template<typename J>
concept int_ish = std::is_integral_v<J> || std::is_enum_v<J>;

struct json_value;
using json = umap<string, shared_ptr<json_value>>;

// JSON lib works by converting everything to and from this type
// Built-in types are declared here, User-types are declared there.
// JSON_IMPL used as a shortcut, where first arg is Type, and the rest are the members to be saved.
struct json_value {
    using json_variant = variant<json, vector<json_value>, string, bool, s64, f64>;
    json_variant value;

    json_value() {}
    template <typename JsonT> explicit json_value(const vector<JsonT>& _vector) {
        vector<json_value> _list = {};
        for (const JsonT& e : _vector) {
            json_value jv  = json_value(e);
            bool       add = true;
            visit(overloaded {
                [&](const json& j) {
                    if (j.size() == 0)
                        add = false;
                },
                [&](const vector<json_value>& jvs) {
                    if (jvs.size() == 0)
                        add = false;
                },
              [](auto a) {}
            }, jv.value);
            if (add)
                _list.insert_back(json_value(e));
        }
        value = json_variant {_list};
    }
    template <typename JsonT> explicit json_value(const vector<shared_ptr<JsonT>>& _vector) {
        vector<json_value> _list = {};
        for (shared_ptr<JsonT> e : _vector)
            _list.insert_back(json_value(*e));
        value = json_variant {_list};
    }
    template <typename JsonT1, typename JsonT2> explicit json_value(umap<JsonT1, JsonT2> _map) {
        vector<json_value> _list = {};
        for (auto& [k, v] : _map) {
            json v_j {};
            v_j["key"]   = make_shared<json_value>(json_value(k));
            v_j["value"] = make_shared<json_value>(json_value(v));
            _list.insert_back(json_value(v_j));
        }
        value = json_variant {_list};
    }

    template <typename JsonT1, typename JsonT2> explicit json_value(umap<shared_ptr<JsonT1>, JsonT2> _map) {
        vector<json_value> _list = {};
        for (auto& [k, v] : _map) {
            json v_j {};
            v_j["key"]   = make_shared<json_value>(json_value(*k));
            v_j["value"] = make_shared<json_value>(json_value(v));
            _list.insert_back(json_value(v_j));
        }
        value = json_variant {_list};
    }

    template <typename JsonT> explicit json_value(const umap<string, JsonT>& _map) {
        json _json {};
        for (auto& [k, v] : _map)
            _json[k] = make_shared<json_value>(json_value(v));
        value = json_variant {_json};
    }

    explicit json_value(vector<json_value> _vector) {
        value = json_variant {_vector};
    }
    explicit json_value(const json& input_json) {
        value = json_variant {input_json};
    }
    explicit json_value(const char* input_string) {
        value = json_variant {string(input_string)};
    }
    explicit json_value(const string& input_string) {
        value = json_variant {input_string};
    }
    explicit json_value(f64 input_float) {
        value = json_variant {input_float};
    }
    explicit json_value(f32 input_float) {
        value = json_variant {(f64) input_float};
    }
    template <int_ish T>
    explicit json_value(T input_int) {
        value = json_variant {(s64) input_int};
    }
    explicit json_value(bool input_bool) {
        value = json_variant {input_bool};
    }

    template <typename JsonT> explicit operator vector<JsonT>() const {
        vector<JsonT>      t;
        vector<json_value> _list = get_list();
        t.reserve(t.size() + _list.size());
        for (auto& e : _list) {
            t.insert_back((JsonT) e);
        }
        return t;
    }
    template <typename JsonT> explicit operator vector<shared_ptr<JsonT>>() const {
        vector<shared_ptr<JsonT>> t;
        vector<json_value>        _list = get_list();
        t.reserve(t.size() + _list.size());
        for (auto& e : _list) {
            t.insert_back(make_shared<JsonT>((JsonT) e));
        }
        return t;
    }

    template <typename JsonS, typename JsonT> explicit operator umap<JsonS, JsonT>() const {
        umap<JsonS, JsonT> t;
        for (auto& value_json_value : get_list()) {
            json& value_json = (json&) value_json_value;
            JsonS key        = (JsonS) *value_json["key"];
            JsonT value      = (JsonT) *value_json["value"];
            t[key]           = value;
        }
        return t;
    }
    template <typename JsonS, typename JsonT> explicit operator umap<shared_ptr<JsonS>, JsonT>() const {
        umap<shared_ptr<JsonS>, JsonT> t;
        for (auto& value_json_value : get_list()) {
            json& value_json = (json&) value_json_value;
            JsonS key        = make_shared<JsonS>(*value_json["key"]);
            JsonT value      = (JsonT) *value_json["value"];
            t[key]           = value;
        }
        return t;
    }

    template <typename JsonT> explicit operator umap<string, JsonT>() const {
        umap<string, JsonT> t;
        for (auto& [k, val] : get<json>(value)) {
            t[k] = JsonT(*val);
        }
        return t;
    }

    explicit operator json() const {
        return get<json>(value);
    }

    explicit operator bool() const {
        return get<bool>(value);
    }

    explicit operator s32() const {
        return (s32) get<s64>(value);
    }
    explicit operator u32() const {
        return (u32) get<s64>(value);
    }
    explicit operator u64() const {
        return (u64) get<s64>(value);
    }
    explicit operator s64() const {
        return get<s64>(value);
    }

    explicit operator f32() const {
        return (f32) get<f64>(value);
    }
    explicit operator f64() const {
        return get<f64>(value);
    }

    explicit operator string() const {
        return get<string>(value);
    }

    vector<json_value> get_list() const {
        return get<vector<json_value>>(value);
    }

    string dump() const;
};

json               parse(string& contents);
json               parse_file(string_view file_name);
json               parse_json(istream& iss);
json_value         parse_item(istream& iss);
vector<json_value> parse_list(istream& iss);
string             parse_quote(istream& iss);

void   file_dump(const json& json, string file_name);
string dump_json(const json& json);
void   delete_json(json& j);

}

#define EXPAND(x) x
#define GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, NAME, ...) NAME
#define PASTE(...) \
    EXPAND(GET_MACRO(__VA_ARGS__, PASTE16, PASTE15, PASTE14, PASTE13, PASTE12, PASTE11, PASTE10, PASTE9, PASTE8, PASTE7, PASTE6, PASTE5, PASTE4, PASTE3, PASTE2, PASTE1)(__VA_ARGS__))
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

#define FROM_JSON_ELE(var_name)                \
    if (j.contains(#var_name)) {             \
        typedef std::conditional<std::is_enum_v<decltype(var_name)>, s32, decltype(var_name)>::type CastableT; \
        using ValueT = decltype(var_name);      \
        (var_name)  = ValueT(CastableT(*j.at(#var_name))); \
    }
#define FROM_JSON_IMPL(Type, ...)                 \
    explicit Type(const spellbook::json_value& jv) {    \
        spellbook::json& j = (spellbook::json&) jv;           \
        EXPAND(PASTE(FROM_JSON_ELE, __VA_ARGS__)) \
    }
#define TO_JSON_ELE(var_name) j[#var_name] = make_shared<spellbook::json_value>(spellbook::json_value(var_name));
#define TO_JSON_IMPL(Type, ...)                 \
    explicit operator spellbook::json_value() const { \
        spellbook::json j = spellbook::json {};             \
        EXPAND(PASTE(TO_JSON_ELE, __VA_ARGS__)) \
        return spellbook::json_value(j);              \
    }
#define JSON_IMPL(Type, ...)          \
    FROM_JSON_IMPL(Type, __VA_ARGS__) \
    TO_JSON_IMPL(Type, __VA_ARGS__)