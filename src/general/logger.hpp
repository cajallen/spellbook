#pragma once

#include <queue>

#include "string.hpp"
#include "color.hpp"

namespace spellbook {

struct BasicMessage {
    string         str;
    string         group      = "None";
    Color          color      = palette::white;
};

extern std::queue<BasicMessage> message_queue;

#define log_warning(msg)                                               \
message_queue.emplace("WARNING: " msg, "warning", palette::orange);

#define log_error(msg)                                            \
message_queue.emplace("ERROR: " msg, "assert", palette::crimson); \
__debugbreak();                                                   \


#define assert_else(cond)              \
if (!(cond)) {                         \
    log_error("ASSERT_FAIL: !(" #cond ")") \
} if (!(cond))

#define check_else(cond)                  \
if (!(cond)) {                            \
    log_warning("CHECK_FAIL: !(" #cond ")"); \
} if (!(cond))



}