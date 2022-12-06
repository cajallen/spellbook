#include "console.hpp"

#include <sstream>
#include <filesystem>
#include <iostream>

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <tracy/Tracy.hpp>

#include "extension/fmt.hpp"
#include "extension/fmt_geometry.hpp"
#include "general/math.hpp"
#include "general/logger.hpp"
#include "game/game.hpp"

namespace fs = std::filesystem;

namespace spellbook {

vector<Message> Console::message_list {};

bool                     Console::input_reset   = false;
bool                     Console::added_message = false;
vector<string>           Console::input_history {};
u32                      Console::input_history_cursor = 0;
umap<string, bool>       Console::group_visible {};
umap<string, bool>       Console::frame_bool {};

void Console::setup() {
    input_history_cursor = input_history.size();
    for (auto& msg : message_list)
        if (group_visible.count(msg.group) == 0)
            group_visible.insert({msg.group, true});

    console({.str = "<<<<<   Console Initialized   >>>>>", .group = "console", .save = false});
}

void console(MsgArg msg) {
    bool frame_ok = msg.frame_tags.size() == 0;
    for (const string& frame_tag : msg.frame_tags) {
        if (Console::frame_bool.count(frame_tag) == 0)
            Console::frame_bool[frame_tag] = false;
        if (Console::frame_bool[frame_tag])
            frame_ok = true;
    }
    if (!frame_ok)
        return;
    Console::_handle_message_queue();
    if (Console::group_visible.count(msg.group) == 0)
        Console::group_visible.insert({msg.group, true});
    if (Console::message_list.size() > 0 && Console::message_list.back() == Message(msg)) {
        Console::message_list.back().count++;
        return;
    }
    const int lazy_size = 10;
    if (Console::message_list.size() > Console::max_messages + lazy_size)
        Console::message_list.remove_indices(0, lazy_size);

    Console::message_list.push_back(Message(msg));
    Console::added_message = true;

    // std::cout << msg.str << std::endl;
}

void console_error(string message, string group, ErrorType type) {
    switch (type) {
        case ErrorType_Notification: {
            console({.str = fmt_("NOTIFICATION: {}", message), .group = group, .color = palette::gray});
        } break;
        case ErrorType_Warning: {
            console({.str = fmt_("WARNING: {}", message), .group = group, .color = palette::orange});
        } break;
        case ErrorType_Severe:
        default: {
            console({.str = fmt_("ERROR: {}", message), .group = group, .color = palette::crimson});
        } break;
    }
}

void Console::show_message(Message& msg) {
    assert_else(group_visible.count(msg.group) == 1);
    if (!group_visible[msg.group])
        return;

    string count = msg.count > 1 ? fmt_("({}) ", msg.count) : "";
    bool   temp  = false;
    if (ImGui::Selectable(("##clipboard" + msg.str).c_str(),
            &temp,
            ImGuiSelectableFlags_SpanAvailWidth | ImGuiSelectableFlags_SelectOnClick,
            ImVec2(0.0f, 0.0f))) {
        ImGui::SetClipboardText(msg.str.c_str());
    }
    ImGui::SameLine();
    ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
    ImGui::TextColored(ImVec4(msg.color), "%s%s", count.c_str(), msg.str.c_str());
    ImGui::PopTextWrapPos();
}

void Console::show_messages(v2i size) {
    ImGui::BeginChild("Messages", (ImVec2) size, true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    for (Message& msg : message_list) {
        show_message(msg);
    }
    if (added_message) {
        ImGui::SetScrollHereY(1.0f);
        added_message = false;
    }
    ImGui::EndChild();
}

int console_input_callback(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion) {
        data->InsertChars(data->CursorPos, "<reproduce and report>");
    } else if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
        if (data->EventKey == ImGuiKey_UpArrow) {
            if (Console::input_history_cursor == 0)
                Console::input_history_cursor = Console::input_history.size();
            else
                Console::input_history_cursor--;

            data->DeleteChars(0, data->BufTextLen);

            if (Console::input_history_cursor != Console::input_history.size()) {
                data->InsertChars(0, Console::input_history[Console::input_history_cursor].c_str());
                data->SelectAll();
            }
        } else if (data->EventKey == ImGuiKey_DownArrow) {
            if (Console::input_history_cursor == Console::input_history.size())
                Console::input_history_cursor = Console::input_history.size();
            else
                Console::input_history_cursor++;

            data->DeleteChars(0, data->BufTextLen);

            if (Console::input_history_cursor != Console::input_history.size()) {
                data->InsertChars(0, Console::input_history[Console::input_history_cursor].c_str());
                data->SelectAll();
            }
        }
    } else if (data->EventFlag == ImGuiInputTextFlags_CallbackAlways) {
        if (Console::input_reset) {
            data->DeleteChars(0, data->BufTextLen);
            Console::input_reset = false;
        }
    }
    return 0;
}

const string tex_exts[]    = {"png", "jpg", "jpeg", "tga", "bmp", "psd", "hdr"};
const string shader_exts[] = {"glsl", "shader", "shad"};
const string vert_exts[]   = {"vert", "glslv", "vsh"};
const string frag_exts[]   = {"frag", "glslf", "fsh"};

void handle_console_input(string& input) {
    Console::input_history.push_back(input);
    Console::input_history_cursor = Console::input_history.size();
    Console::input_reset          = true;

    console({.str = fmt_("> {}", input), .group = "console", .color = palette::green});

    std::istringstream iss(input);
    string             word;
    iss >> word;
    if (word == "help") {
        console({.str = "None to provide", .group = "console"});
    } else if (word == "load") {
        string rest;
        getline(iss, rest);
        //assets::convert(rest, "resources", "resources");
    } else if (word == "cwd") {
        console({.str = fmt_("{}\n", fs::current_path().string()), .group = "console"});
    } else if (word == "ls") {
        auto path = fs::current_path();
        if (iss.good()) {
            iss >> word;
            path /= word;
        }
        for (const auto& entry : fs::directory_iterator(path))
            console({.str = fmt_("{}\n", entry.path().string()), .group = "console"});
    } else if (word == "convert") {
        iss >> word;
       //assets::convert(fs::path(word), "resources", "resources");
    } else if (word == "call") {
        Console::_handle_call_request(iss);
    } else if (word == "clear") {
        Console::message_list.clear();
        Console::input_history.clear();
        Console::input_history_cursor = Console::input_history.size();
    } else {
        console({.str = fmt_("Unknown input: {}", input), .group = "console", .color = palette::orange});
    }
}

void Console::window(bool* open) {
    ZoneScoped;
    _handle_message_queue();
    
    for (auto& [k, v] : frame_bool) {
        v = false;
    }
    if (ImGui::Begin("Console", open, ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Clear Messages")) {
                message_list.clear();
                ImGui::CloseCurrentPopup();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Clear Input History")) {
                input_history.clear();
                input_history_cursor = input_history.size();
                ImGui::CloseCurrentPopup();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Groups")) {
                if (ImGui::MenuItem("Show all", "", nullptr)) {
                    for (auto& [key, value] : group_visible)
                        value = true;
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::MenuItem("Hide all", "", nullptr)) {
                    for (auto& [key, value] : group_visible)
                        value = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::Separator();
                for (auto& [key, value] : group_visible)
                    ImGui::MenuItem(key.c_str(), nullptr, &value);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Frame Print")) {
                for (auto& [key, value] : frame_bool) {
                    if (ImGui::MenuItem(key.c_str(), "", &value)) {
                        value = true;
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
        int messages_size = ImGui::GetContentRegionAvail().y;
        messages_size -= ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y + 2;
        show_messages({0, messages_size});

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        static string console_input;
        auto flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackAlways;
        if (ImGui::InputText("##ConsoleInput", &console_input, flags, console_input_callback)) {
            handle_console_input(console_input);
            ImGui::SetActiveID(ImGui::GetID("##ConsoleInput"), ImGui::GetCurrentWindow());
        }
    }
    ImGui::End();
}

void Console::_handle_message_queue() {
    while (!message_queue.empty()) {
        BasicMessage& message = message_queue.front();
        console({.str = message.str, .group = message.group, .color = message.color});
    }
}

string string2string(string_view word) {
    return string(word);
}
int string2int(const string& word) {
    return std::stoi(word);
}
float string2float(const string& word) {
    return std::stof(word);
}
bool string2bool(string_view word) {
    if (word == "true")
        return true;
    if (word == "false")
        return false;
    console({.str = "false assumed", .color = palette::orange});
    return false;
}

#define HOOK_FUNCTION_CASE1(func, t_var1)                                                   \
    if (func_name == #func) {                                                               \
        string word;                                                                        \
        iss >> word;                                                                        \
        t_var1 var1 = string2##t_var1(word);                                                \
        console({.str = fmt_("{}({}) = {}", #func, var1, func(var1)), .group = "console"}); \
        return;                                                                             \
    }
#define HOOK_FUNCTION_CASE2(func, t_var1, t_var2)                                                          \
    if (func_name == #func) {                                                                              \
        string word;                                                                                       \
        iss >> word;                                                                                       \
        t_var1 var1 = string2##t_var1(word);                                                               \
        iss >> word;                                                                                       \
        t_var2 var2 = string2##t_var2(word);                                                               \
        console({.str = fmt_("{}({},{}) = {}", #func, var1, var2, func(var1, var2)), .group = "console"}); \
        return;                                                                                            \
    }
#define HOOK_FUNCTION_CASE3(func, t_var1, t_var2, t_var3)                                                                 \
    if (func_name == #func) {                                                                                             \
        string word;                                                                                                      \
        iss >> word;                                                                                                      \
        t_var1 var1 = string2##t_var1(word);                                                                              \
        iss >> word;                                                                                                      \
        t_var2 var2 = string2##t_var2(word);                                                                              \
        iss >> word;                                                                                                      \
        t_var3 var3 = string2##t_var3(word);                                                                              \
        console({.str = fmt_("{}({},{},{}) = {}", #func, var1, var2, var3, func(var1, var2, var3)), .group = "console"}); \
        return;                                                                                                           \
    }
#define HOOK_FUNCTION_CASE4(func, t_var1, t_var2, t_var3, t_var4)                                                                        \
    if (func_name == #func) {                                                                                                            \
        string word;                                                                                                                     \
        iss >> word;                                                                                                                     \
        t_var1 var1 = string2##t_var1(word);                                                                                             \
        iss >> word;                                                                                                                     \
        t_var2 var2 = string2##t_var2(word);                                                                                             \
        iss >> word;                                                                                                                     \
        t_var3 var3 = string2##t_var3(word);                                                                                             \
        iss >> word;                                                                                                                     \
        t_var4 var4 = string2##t_var4(word);                                                                                             \
        console({.str = fmt_("{}({},{},{},{}) = {}", #func, var1, var2, var3, var4, func(var1, var2, var3, var4)), .group = "console"}); \
        return;                                                                                                                          \
    }

void Console::_handle_call_request(std::istringstream& iss) {
    string func_name;
    iss >> func_name;

    // custom types need a "string2type" method, and fmt support
    HOOK_FUNCTION_CASE1(math::euler2vector, euler);
}

}