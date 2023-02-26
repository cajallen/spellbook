#include "impair.hpp"

namespace spellbook {

bool Impairs::is_impaired(ImpairType type) {
    for (auto& [_, active_type] : boolean_impairs)
        if (type == active_type)
            return true;
    return false;
}

}