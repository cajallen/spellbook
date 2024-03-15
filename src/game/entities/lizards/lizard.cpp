#include "lizard.hpp"

#include "general/file/file_cache.hpp"

namespace spellbook {

LizardDatabase& get_lizard_database() {
    static LizardDatabase database;
    if (!database.initialized)
        initialize_lizard_database(database);
    return database;
}

void initialize_lizard_database(LizardDatabase& database) {
    json& lizards_json = get_file_cache().load_json("resources/data/lizards.sbjson"_content);

    database.lizard_entries.resize(magic_enum::enum_count<LizardType>());
    database.ability_entries.resize(magic_enum::enum_count<AbilityType>() * magic_enum::enum_count<LizardType>());
    for (const auto& lizard_jv_ptr : lizards_json.at("List")->get_list()) {
        LizardDatabase::LizardEntry entry {};
        json lizard_json = from_jv<json>(lizard_jv_ptr);
        for (const auto& [value_name, value_jv_ptr] : lizard_json) {
            if (value_name == "Type") {
                entry.type = from_jv<LizardType>(*value_jv_ptr);
                continue;
            }
            if (value_name == "Name") {
                entry.name = from_jv<string>(*value_jv_ptr);
                continue;
            }
            if (value_name == "Color") {
                entry.color = Color32(from_jv<string>(*value_jv_ptr));
                continue;
            }
            if (value_name == "Scale") {
                entry.scale = from_jv<float>(*value_jv_ptr);
                continue;
            }
            if (value_name == "Model") {
                entry.model_path = FilePath(from_jv<string>(*value_jv_ptr), FilePathLocation_Content);
                continue;
            }
            if (value_name == "HurtEmitter") {
                entry.hurt_emitter = FilePath(from_jv<string>(*value_jv_ptr), FilePathLocation_Content);
                continue;
            }
            visit(overloaded{
                [&](float arg) { entry.extra_floats[value_name] = arg; },
                [&](const string& arg) { entry.extra_strings[value_name] = arg; },
                [](auto arg) {}
            }, value_jv_ptr->value);
        }
        database.lizard_entries[entry.type] = entry;
    }

    json& abilities_json = get_file_cache().load_json("resources/data/lizard_abilities.sbjson"_content);
    for (const auto& ability_jv_ptr : abilities_json.at("List")->get_list()) {
        LizardDatabase::AbilityEntry entry {};
        json ability_json = from_jv<json>(ability_jv_ptr);
        for (const auto& [value_name, value_jv_ptr] : ability_json) {
            if (value_name == "Lizard") {
                entry.lizard_type = from_jv<LizardType>(*value_jv_ptr);
                continue;
            }
            if (value_name == "Type") {
                entry.ability_type = from_jv<AbilityType>(*value_jv_ptr);
                continue;
            }
            if (value_name == "Name") {
                entry.name = from_jv<string>(*value_jv_ptr);
                continue;
            }
            if (value_name == "Description") {
                entry.description = from_jv<string>(*value_jv_ptr);
                continue;
            }
            if (value_name == "Pre") {
                entry.pre_time = from_jv<float>(*value_jv_ptr);
                continue;
            }
            if (value_name == "Post") {
                entry.post_time = from_jv<float>(*value_jv_ptr);
                continue;
            }
            visit(overloaded{
                [&](double arg) {
                    entry.extra_floats[value_name] = arg;
                },
                [&](const string& arg) {
                    entry.extra_strings[value_name] = arg;
                },
                [](auto arg) {}
            }, value_jv_ptr->value);
        }
        database.ability_entries[ability_index(entry.lizard_type, entry.ability_type)] = entry;
    }
    for (uint32 i = 0; i < magic_enum::enum_count<LizardType>(); i++) {
        auto& attack_entry = database.ability_entries[ability_index(LizardType(i), AbilityType_Attack)];
        auto& spell_entry = database.ability_entries[ability_index(LizardType(i), AbilityType_Spell)];

        attack_entry.extra_strings["Spell"] = spell_entry.name;
        spell_entry.extra_strings["Attack"] = attack_entry.name;
    }
    database.initialized = true;
}

}