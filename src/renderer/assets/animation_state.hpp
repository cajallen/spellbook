#pragma once

namespace spellbook {

enum AnimationState {
    AnimationState_Idle,
    AnimationState_Flail,
    AnimationState_Walking,
    AnimationState_AttackInto,
    AnimationState_AttackOut,
    AnimationState_AttackAgain,
    AnimationState_CastInto,
    AnimationState_CastOut,
    AnimationState_CastAgain,
    AnimationStateCount
};

}