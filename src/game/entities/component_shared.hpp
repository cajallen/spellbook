#pragma once

#include "general/math/geometry.hpp"

namespace spellbook {

bool simple_inspect(const char* tag, float* f);
bool simple_inspect(const char* tag, v2* v);
bool simple_inspect(const char* tag, v3* v);
bool simple_inspect(const char* tag, int* f);
bool simple_inspect(const char* tag, v2i* v);
bool simple_inspect(const char* tag, v3i* v);
bool simple_inspect(const char* tag, euler* s);
bool simple_inspect(const char* tag, string* s);

}

