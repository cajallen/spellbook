#pragma once

#include "general/string.hpp"
#include "general/vector.hpp"
#include "general/umap.hpp"
#include "general/color.hpp"
#include "general/file/json.hpp"

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
#define vk_check(cond)                                                                               \
    if (cond != 0) {                                                                                 \
        console({.str = "VKCHECK: (" #cond ") != 0", .group = "assert", .color = palette::crimson}); \
        __debugbreak();                                                                              \
        abort();                                                                                     \
    }

enum ErrorType { ErrorType_None, ErrorType_Notification, ErrorType_Warning, ErrorType_Severe };
void console_error(string message, string group, ErrorType type);

struct Console {
    static constexpr int max_messages = 200;

    static vector<Message> message_list;

    static bool                     input_reset;
    static bool                     added_message;
    static vector<string>           input_history;
    static uint32                      input_history_cursor;
    static umap<string, bool>       group_visible;
    static umap<string, bool>       frame_bool;

    static void setup();
    static void window(bool* p_open);
    static void show_message(Message& msg);
    static void show_messages(v2i size);
    static void _handle_call_request(std::istringstream& iss);
    static void _handle_message_queue();
};

JSON_IMPL(Console, Console::input_history, Console::message_list, Console::group_visible);

}
