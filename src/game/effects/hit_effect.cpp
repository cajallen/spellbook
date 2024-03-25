#include "hit_effect.hpp"

#include "game/scene.hpp"
#include "game/entities/components.hpp"
#include "game/entities/enemy.hpp"
#include "game/entities/caster.hpp"

namespace spellbook {

void damage_apply(HitEffect& effect, Scene* scene, entt::entity caster, const vector<entt::entity>& targets) {
    LogicTransform* source_tfm = scene->registry.try_get<LogicTransform>(caster);
    v3 source_pos = {};
    if (source_tfm)
        source_pos = source_tfm->position;

    for (entt::entity target : targets) {
        Health& health = scene->registry.get<Health>(target);
        LogicTransform& enemy_tfm = scene->registry.get<LogicTransform>(target);
        health.damage(caster, effect.quantity / targets.size(), enemy_tfm.position - source_pos);
    }
}

void heal_apply(HitEffect& effect, Scene* scene, entt::entity caster, const vector<entt::entity>& targets) {
    for (entt::entity target : targets) {
        Health& health = scene->registry.get<Health>(target);
        health.heal(caster, effect.quantity / targets.size());
    }
}

void regen_apply(HitEffect& effect, Scene* scene, entt::entity caster, const vector<entt::entity>& targets) {
    EffectDatabase::EffectEntry& entry = get_effect_database().entries[effect.type];
    for (entt::entity target : targets) {
        Health* health = scene->registry.try_get<Health>(target);
        unique_ptr<Stat>& r = health->regen;

        StatEffect stat_effect = {
            .type = StatEffect::Type_Add,
            .value = effect.quantity,
            .until = Input::time + effect.duration,
            .unique = uint64(effect.type)
        };
        r->add_effect(uint64(effect.type) ^ uint64(caster), stat_effect);
    }
}

void growth_apply(HitEffect& effect, Scene* scene, entt::entity caster, const vector<entt::entity>& targets) {
    EffectDatabase::EffectEntry& entry = get_effect_database().entries[effect.type];
    for (entt::entity target : targets) {
        Health* health = scene->registry.try_get<Health>(target);
        unique_ptr<Stat>& mh = health->max_health;

        StatEffect stat_effect = {
            .type = StatEffect::Type_Add,
            .value = effect.quantity,
            .until = Input::time + effect.duration,
            .unique = uint64(effect.type)
        };
        mh->add_effect(uint64(effect.type) ^ uint64(caster), stat_effect);
    }
}

void vulnerable_apply(HitEffect& effect, Scene* scene, entt::entity caster, const vector<entt::entity>& targets) {
    EffectDatabase::EffectEntry& entry = get_effect_database().entries[effect.type];
    for (entt::entity target : targets) {
        Health* health = scene->registry.try_get<Health>(target);
        unique_ptr<Stat>& dtm = health->damage_taken_multiplier;

        StatEffect stat_effect = {
            .type = StatEffect::Type_Multiply,
            .value = 1.0f + entry.extra_floats.at(fmt_("Level{}", effect.level)),
            .until = Input::time + effect.duration,
            .unique = uint64(effect.type)
        };
        dtm->add_effect(uint64(effect.type) ^ uint64(caster), stat_effect);
    }
}

void resistance_apply(HitEffect& effect, Scene* scene, entt::entity caster, const vector<entt::entity>& targets) {
    EffectDatabase::EffectEntry& entry = get_effect_database().entries[effect.type];
    for (entt::entity target : targets) {
        Health* health = scene->registry.try_get<Health>(target);
        unique_ptr<Stat>& dtm = health->damage_taken_multiplier;

        StatEffect stat_effect = {
            .type = StatEffect::Type_Multiply,
            .value = 1.0f - entry.extra_floats.at(fmt_("Level{}", effect.level)),
            .until = Input::time + effect.duration,
            .unique = uint64(effect.type)
        };
        dtm->add_effect(uint64(effect.type) ^ uint64(caster), stat_effect);
    }
}

void slow_apply(HitEffect& effect, Scene* scene, entt::entity caster, const vector<entt::entity>& targets) {
    EffectDatabase::EffectEntry& entry = get_effect_database().entries[effect.type];
    for (entt::entity target : targets) {
        Traveler* move = scene->registry.try_get<Traveler>(target);
        unique_ptr<Stat>& speed = move->max_speed;

        StatEffect stat_effect = {
            .type = StatEffect::Type_Multiply,
            .value = 1.0f - entry.extra_floats.at(fmt_("Level{}", effect.level)),
            .until = Input::time + effect.duration,
            .unique = uint64(effect.type)
        };
        speed->add_effect(uint64(effect.type) ^ uint64(caster), stat_effect);
    }
}

void haste_apply(HitEffect& effect, Scene* scene, entt::entity caster, const vector<entt::entity>& targets) {
    EffectDatabase::EffectEntry& entry = get_effect_database().entries[effect.type];
    for (entt::entity target : targets) {
        Caster* enemy_caster = scene->registry.try_get<Caster>(target);
        unique_ptr<Stat>& cast_speed = enemy_caster->attack_speed;
        unique_ptr<Stat>& cooldown_speed = enemy_caster->cooldown_speed;

        StatEffect stat_effect = {
            .type = StatEffect::Type_Multiply,
            .value = 1.0f - entry.extra_floats.at(fmt_("Level{}", effect.level)),
            .until = Input::time + effect.duration,
            .unique = uint64(effect.type)
        };
        cast_speed->add_effect(uint64(effect.type) ^ uint64(enemy_caster), stat_effect);
        cooldown_speed->add_effect(uint64(effect.type) ^ uint64(enemy_caster), stat_effect);
    }
}

void poison_apply(HitEffect& effect, Scene* scene, entt::entity caster, const vector<entt::entity>& targets) {
    EffectDatabase::EffectEntry& entry = get_effect_database().entries[effect.type];
    for (entt::entity target : targets) {
        Caster* caster = scene->registry.try_get<Caster>(target);
    }
}

bool damage_hits_disposition(Team::Disposition disposition) {
    switch (disposition) {
        case Team::Disposition_Friendly: return false;
        case Team::Disposition_Hostile: return true;
        default: return false;
    }
}

bool heal_hits_disposition(Team::Disposition disposition) {
    switch (disposition) {
        case Team::Disposition_Friendly: return true;
        case Team::Disposition_Hostile: return false;
        default: return false;
    }
}

bool growth_hits_disposition(Team::Disposition disposition) {
    switch (disposition) {
        case Team::Disposition_Friendly: return true;
        case Team::Disposition_Hostile: return false;
        default: return false;
    }
}

bool regen_hits_disposition(Team::Disposition disposition) {
    switch (disposition) {
        case Team::Disposition_Friendly: return true;
        case Team::Disposition_Hostile: return false;
        default: return false;
    }
}

bool vulnerable_hits_disposition(Team::Disposition disposition) {
    switch (disposition) {
        case Team::Disposition_Friendly: return false;
        case Team::Disposition_Hostile: return true;
        default: return false;
    }
}

bool resistance_hits_disposition(Team::Disposition disposition) {
    switch (disposition) {
        case Team::Disposition_Friendly: return true;
        case Team::Disposition_Hostile: return false;
        default: return false;
    }
}

bool slow_hits_disposition(Team::Disposition disposition) {
    switch (disposition) {
        case Team::Disposition_Friendly: return false;
        case Team::Disposition_Hostile: return true;
        default: return false;
    }
}

bool haste_hits_disposition(Team::Disposition disposition) {
    switch (disposition) {
        case Team::Disposition_Friendly: return true;
        case Team::Disposition_Hostile: return false;
        default: return false;
    }
}

bool poison_hits_disposition(Team::Disposition disposition) {
    switch (disposition) {
        case Team::Disposition_Friendly: return false;
        case Team::Disposition_Hostile: return true;
        default: return false;
    }
}

HitEffect::HitEffect(EffectType _type) {
    type = _type;
    switch (type) {
        case (EffectType_Damage): {
            apply_function = damage_apply;
            hits_disposition_function = damage_hits_disposition;
        } break;
        case (EffectType_Heal): {
            apply_function = heal_apply;
            hits_disposition_function = heal_hits_disposition;
        } break;
        case (EffectType_Growth): {
            apply_function = growth_apply;
            hits_disposition_function = growth_hits_disposition;
        } break;
        case (EffectType_Regen): {
            apply_function = regen_apply;
            hits_disposition_function = regen_hits_disposition;
        } break;
    }
}

EffectDatabase& get_effect_database() {
    static EffectDatabase database;
    if (!database.initialized)
        initialize_effect_database(database);
    return database;
}

void initialize_effect_database(EffectDatabase& database) {
    json& database_json = get_file_cache().load_json("resources/data/effects.sbjson"_content);

    database.entries.resize(magic_enum::enum_count<EffectType>());
    for (const auto& effect_jv_ptr : database_json.at("List")->get_list()) {
        EffectDatabase::EffectEntry entry {};
        json effect_json = from_jv<json>(effect_jv_ptr);
        EffectType type;
        for (const auto& [value_name, value_jv_ptr] : effect_json) {
            if (value_name == "Type") {
                type = from_jv<EffectType>(*value_jv_ptr);
                continue;
            }
            if (value_name == "Description") {
                entry.description = from_jv<string>(*value_jv_ptr);
                continue;
            }
            if (value_name == "Color") {
                entry.color = Color32(from_jv<string>(*value_jv_ptr));
                continue;
            }
            visit(overloaded{
                [&](double arg) { entry.extra_floats[value_name] = arg; },
                [&](const string& arg) { entry.extra_strings[value_name] = arg; },
                [](auto arg) {}
            }, value_jv_ptr->value);
        }
        database.entries[uint64(type)] = entry;
    }
    database.initialized = true;
}

}