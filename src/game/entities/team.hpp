#pragma once

namespace spellbook {

struct Team {
    enum TeamID : uint8 {
        Team_None,
        Team_Lizards,
        Team_Bots
    };
    TeamID identity = Team_None;

    enum Disposition : uint8 {
        Disposition_Friendly,
        Disposition_Neutral,
        Disposition_Hostile
    };

    Disposition outlook[8];
};

}