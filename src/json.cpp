#include "json.hpp"

#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

#include "dtoa.h"

using std::visit;
using std::ifstream;
using std::istringstream;
using std::ofstream;
using std::ostringstream;
using std::to_string;

namespace spellbook {

json parse(string& contents) {
    istringstream iss(contents);
    return parse_json(iss);
}

json parse_file(const string& file_name) {
    ifstream ifs(file_name);
    if (ifs.is_open())
        return parse_json(ifs);
    return json {};
}

json parse_json(istream& iss) {
    json   j;
    string word;
    char   c;
    iss.get(c);
    if (c != '{')
        __debugbreak(); // invalid start
    iss.unget();

    while (iss) {
        // skip bullshit
        iss.get(c);
        while ((c == ',' || c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\f' || c == '{') && iss.good()) {
            iss.get(c);
        }
        if (c == '}')
            return j;
        // don't unget because we expect key, parse_quote skips start "

        word = parse_quote(iss);
        iss.get(c);
        while ((c == ',' || c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\f' || c == ':') && iss.good()) {
            iss.get(c);
        }
        iss.unget();
        j.insert({word, make_shared<json_value>(parse_item(iss))});
    }
    __debugbreak(); // missing end
    return j;
}

bool is_int(const string& s) {
    return !s.empty() && find_if(s.begin(), s.end(), [](u8 c) {
        return !(isdigit(c) || c == '-');
    }) == s.end();
}

bool is_float(const string& s) {
    return !s.empty() && find_if(s.begin(), s.end(), [](u8 c) {
        return !(isdigit(c) || c == '.' || c == '-' || c == 'e');
    }) == s.end();
}
json_value parse_item(istream& iss) {
    char c = ' ';
    iss.get(c);
    while ((c == ',' || c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\f') && iss.good())
        iss.get(c);
    if (c == '{') {
        iss.unget();
        return to_jv(parse_json(iss));
    } else if (c == '[') {
        return to_jv(parse_list(iss));
    } else if (c == '"') {
        return to_jv(parse_quote(iss));
    } else {
        if (isalpha(c)) {
            string word;
            while (isalpha(c)) {
                word.push_back(c);
                iss.get(c);
            }
            iss.unget();
            if (word.rfind("true", 0) == 0)
                return to_jv(true);
            if (word.rfind("false", 0) == 0)
                return to_jv(false);
        } else {
            string word;
            while (isdigit(c) || c == '.' || c == '-' || c == 'e') {
                word.push_back(c);
                iss.get(c);
            }
            iss.unget();
            if (is_int(word))
                return to_jv(stoll(word));
            if (is_float(word)) {
                return to_jv(stod(word));
            }
        }
    }
    return json_value();
}

vector<json_value> parse_list(istream& iss) {
    vector<json_value> values;
    char               c;
    while (iss) {
        iss.get(c);
        while ((c == ',' || c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\f' || c == ']') && iss.good()) {
            if (c == ']')
                return values;
            iss.get(c);
        }
        iss.unget();
        values.insert_back(parse_item(iss));
    }
    __debugbreak(); // no closing brace
    return values;
}

string parse_quote(istream& iss) {
    string out;
    while (iss) {
        char c;
        iss.get(c);
        if (c == '\\') {
            iss.get(c);
            out.push_back(c);
        } else {
            if (c == '"')
                break;
            out.push_back(c);
        }
    }
    return out;
}

void file_dump(const json& json, string file_name) {
    ofstream ofs(file_name);
    ofs << dump_json(json);
}

string dump_json(const json& json) {
    ostringstream oss;
    oss << "{";
    bool init = false;
    for (auto& [key, json_value] : json) {
        if (init)
            oss << ",";
        oss << "\"" << key << "\":" << json_value->dump();
        init = true;
    }
    oss << "}";
    return oss.str();
}

string json_value::dump() const {
    string s;
    visit(overloaded {[&s](bool a) {
                          s = a ? "true" : "false";
                      },
              [&s](s64 a) {
                  s = to_string(a);
              },
              [&s](f64 a) {
                  static char buffer[256];
                  auto        size = dtoa(a, buffer);
                  s                = string(buffer, size);
              },
              [&s](const string& a) {
                  std::ostringstream oss;
                  oss << std::quoted(a);
                  s = oss.str();
              },
              [&s](const json& a) {
                  s = dump_json(a);
              },
              [&s](const vector<json_value>& a) {
                  s         = "[";
                  bool init = false;
                  for (auto jv : a) {
                      if (init)
                          s += ",";
                      s += jv.dump();
                      init = true;
                  }
                  s += "]";
              }},
        value);
    return s;
}

} // namespace caj
