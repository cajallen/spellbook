#pragma once

#include <fmt/core.h>

#include "string.hpp"
#include "vector.hpp"

#include "color.hpp"
#include "json.hpp"
#include "umap.hpp"

#define fmt_ fmt::format

namespace spellbook {

struct MsgArg {
    string         str;
    string         group      = "None";
    Color          color      = palette::white;
    bool           save       = true;
    vector<string> frame_tags = {};
    int            count      = 1;
};

struct Message {
    string str;
    string group = "None";
    Color  color = palette::white;
    bool   save  = true;
    int    count = 1;

    Message() = default;
    Message(const MsgArg& arg) : str(arg.str), group(arg.group), color(arg.color), save(arg.save), count(arg.count) {}
    bool operator==(Message& rhs) const {
        return str == rhs.str && color == rhs.color && group == rhs.group;
    }
    
};

FROM_JSON_IMPL(Message, str, count, color, group)
inline json_value to_jv(const Message& value) {
    auto j = json();
    if (!value.save)
        return {};
    TO_JSON_ELE(str);
    TO_JSON_ELE(group);
    TO_JSON_ELE(color);
    TO_JSON_ELE(count);
    return to_jv(j);
}

void console(MsgArg content);
#define assert_else(cond)                                                                   \
    if (!(cond)) {                                                                          \
        console({.str = "ASSERT_FAIL:" #cond, .group = "assert", .color = palette::crimson}); \
        __debugbreak();                                                                     \
    }                                                                                       \
    if (!(cond))
#define fmt_assert_else(cond, fstr, ...)                                                              \
    if (!(cond)) {                                                                                    \
        console({.str = "ASSERT_FAIL:" #cond, .group = "assert", .color = palette::crimson});           \
        console({.str = fmt::format(fstr, __VA_ARGS__), .group = "assert", .color = palette::crimson}); \
        __debugbreak();                                                                               \
    }                                                                                                 \
    if (!(cond))
#define vk_check(cond)                                                                  \
    if (cond != 0) {                                                                    \
        console({.str = "VKCHECK:" #cond, .group = "assert", .color = palette::crimson}); \
        __debugbreak();                                                                 \
        abort();                                                                        \
    }

enum ErrorType { ErrorType_None, ErrorType_Notification, ErrorType_Warning, ErrorType_Severe };
void console_error(string message, string group, ErrorType type);

struct Console {
    static constexpr int max_messages = 200;

    static vector<Message> message_list;

    static bool                     input_reset;
    static bool                     added_message;
    static vector<string>           input_history;
    static string*                  input_history_cursor;
    static umap<string, bool>       group_visible;
    static umap<string, bool>       frame_bool;

    static void setup();
    static void window(bool* p_open);
    static void show_message(Message& msg);
    static void show_messages(v2i size);
    static void _handle_call_request(std::istringstream& iss);
};

JSON_IMPL(Console, Console::input_history, Console::message_list, Console::group_visible);

}
