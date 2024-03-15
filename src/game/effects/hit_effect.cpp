#include "hit_effect.hpp"

#include "game/scene.hpp"
#include "game/entities/components.hpp"
#include "game/entities/enemy.hpp"
#include "game/entities/caster.hpp"

namespace spellbook {

void Damage::apply(Scene* scene, entt::entity caster, const vector<entt::entity>& targets) {
    for (entt::entity target : targets) {
        Caster* caster = scene->registry.try_get<Caster>(target);
    }
}

void Vulnerable::apply(Scene* scene, entt::entity caster, const vector<entt::entity>& targets) {
    for (entt::entity target : targets) {
        Health* health = scene->registry.try_get<Health>(target);
        unique_ptr<Stat>& dtm = health->damage_taken_multiplier;

        StatEffect stat_effect = {
            .type = StatEffect::Type_Multiply,
            .value = 1.0f + effect[level - 1],
            .until = Input::time + duration,
            .unique = hash_view("Vulnerable")
        };
        dtm->add_effect(hash_view("Vulnerable") ^ uint64(caster), stat_effect);
    }
}

void Resistance::apply(Scene* scene, entt::entity caster, const vector<entt::entity>& targets) {
    for (entt::entity target : targets) {
        Health* health = scene->registry.try_get<Health>(target);
        unique_ptr<Stat>& dtm = health->damage_taken_multiplier;

        StatEffect stat_effect = {
            .type = StatEffect::Type_Multiply,
            .value = 1.0f - effect[level - 1],
            .until = Input::time + duration,
            .unique = hash_view("Resistance")
        };
        dtm->add_effect(hash_view("Resistance") ^ uint64(caster), stat_effect);
    }
}

void Slow::apply(Scene* scene, entt::entity caster, const vector<entt::entity>& targets) {
    for (entt::entity target : targets) {
        Traveler* move = scene->registry.try_get<Traveler>(target);
        unique_ptr<Stat>& speed = move->max_speed;

        StatEffect stat_effect = {
            .type = StatEffect::Type_Multiply,
            .value = 1.0f - effect[level - 1],
            .until = Input::time + duration,
            .unique = hash_view("Slow")
        };
        speed->add_effect(hash_view("Slow") ^ uint64(caster), stat_effect);
    }
}

void Haste::apply(Scene* scene, entt::entity caster, const vector<entt::entity>& targets) {
    for (entt::entity target : targets) {
        Caster* caster = scene->registry.try_get<Caster>(target);
        unique_ptr<Stat>& cast_speed = caster->attack_speed;
        unique_ptr<Stat>& cooldown_speed = caster->cooldown_speed;

        StatEffect stat_effect = {
            .type = StatEffect::Type_Multiply,
            .value = 1.0f - effect[level - 1],
            .until = Input::time + duration,
            .unique = hash_view("Haste")
        };
        cast_speed->add_effect(hash_view("Haste") ^ uint64(caster), stat_effect);
        cooldown_speed->add_effect(hash_view("Haste") ^ uint64(caster), stat_effect);
    }
}

void Poison::apply(Scene* scene, entt::entity caster, const vector<entt::entity>& targets) {
    for (entt::entity target : targets) {
        Caster* caster = scene->registry.try_get<Caster>(target);
    }
}

void Growth::apply(Scene* scene, entt::entity caster, const vector<entt::entity>& targets) {
    for (entt::entity target : targets) {
        Caster* caster = scene->registry.try_get<Caster>(target);
    }
}

bool Damage::hits_disposition(Team::Disposition disposition) {
    switch (disposition) {
        case Team::Disposition_Friendly: return false;
        case Team::Disposition_Hostile: return true;
        default: return false;
    }
}

bool Vulnerable::hits_disposition(Team::Disposition disposition) {
    switch (disposition) {
        case Team::Disposition_Friendly: return false;
        case Team::Disposition_Hostile: return true;
        default: return false;
    }
}

bool Resistance::hits_disposition(Team::Disposition disposition) {
    switch (disposition) {
        case Team::Disposition_Friendly: return true;
        case Team::Disposition_Hostile: return false;
        default: return false;
    }
}

bool Slow::hits_disposition(Team::Disposition disposition) {
    switch (disposition) {
        case Team::Disposition_Friendly: return false;
        case Team::Disposition_Hostile: return true;
        default: return false;
    }
}

bool Haste::hits_disposition(Team::Disposition disposition) {
    switch (disposition) {
        case Team::Disposition_Friendly: return true;
        case Team::Disposition_Hostile: return false;
        default: return false;
    }
}

bool Poison::hits_disposition(Team::Disposition disposition) {
    switch (disposition) {
        case Team::Disposition_Friendly: return false;
        case Team::Disposition_Hostile: return true;
        default: return false;
    }
}

bool Growth::hits_disposition(Team::Disposition disposition) {
    switch (disposition) {
        case Team::Disposition_Friendly: return true;
        case Team::Disposition_Hostile: return false;
        default: return false;
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
    for (const auto& [effect_name, effect_jv_ptr] : database_json) {
        EffectDatabase::EffectEntry entry {.name = effect_name};
        json effect_json = from_jv<json>(*effect_jv_ptr);
        for (const auto& [value_name, value_jv_ptr] : effect_json) {
            if (value_name == "Description") {
                entry.description = from_jv<string>(*value_jv_ptr);
                entry.extra_strings[value_name] = entry.description;
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
        database.entries[hash_view(effect_name)] = entry;
    }
    database.initialized = true;
}

}