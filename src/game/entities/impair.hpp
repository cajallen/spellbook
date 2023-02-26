#pragma once

#include "general/umap.hpp"

namespace spellbook {

enum StaticImpairSources : u64 {
    ImpairSource_Dragging
};

enum ImpairType : u8 {
    ImpairType_NoMove,
    ImpairType_NoCast
};

struct Impairs {
    umap<u64, ImpairType> boolean_impairs;

    bool is_impaired(ImpairType);
};

}

