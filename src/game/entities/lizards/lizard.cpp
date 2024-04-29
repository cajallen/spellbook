#include "lizard.hpp"

#include "general/file/file_cache.hpp"
#include "game/scene.hpp"
#include "game/pose_controller.hpp"
#include "game/text_3d.hpp"
#include "game/entities/components.hpp"
#include "game/entities/caster.hpp"
#include "game/entities/team.hpp"
#include "game/entities/tags.hpp"

namespace spellbook {

LizardDatabase& get_lizard_database() {
    static LizardDatabase database;
    if (!database.initialized)
        initialize_lizard_database(database);
    return database;
}

void initialize_lizard_database(LizardDatabase& database) {
    json& lizards_json = get_file_cache().load_json("resources/data/lizards.sbjson"_content);

    database.lizard_entries.resize(magic_enum::enum_count<LizardType>() * LIZARD_VARIANT_COUNT);
    database.ability_entries.resize(magic_enum::enum_count<AbilityType>() * magic_enum::enum_count<LizardType>() * LIZARD_VARIANT_COUNT);
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
            if (value_name == "Background") {
                entry.background = FilePath(from_jv<string>(*value_jv_ptr), FilePathLocation_Content);
                continue;
            }
            if (value_name == "Variants") {
                uint8 variant_index = 0;
                for (const auto& variant_jv_ptr : value_jv_ptr->get_list()) {
                    json variant_json = from_jv<json>(variant_jv_ptr);
                    for (const auto& [variant_name, variant_jv_ptr] : variant_json) {
                        if (variant_name == "Title") {
                            entry.variants[variant_index].title = from_jv<string>(*variant_jv_ptr);
                            continue;
                        }
                        if (variant_name == "Splash") {
                            entry.variants[variant_index].splash = FilePath(from_jv<string>(*variant_jv_ptr), FilePathLocation_Content);
                            continue;
                        }
                        if (variant_name == "Color") {
                            entry.variants[variant_index].color = Color32(from_jv<string>(*variant_jv_ptr));
                            continue;
                        }
                        if (variant_name == "UV") {
                            entry.variants[variant_index].uv = uint8(from_jv<int32>(*variant_jv_ptr));
                            continue;
                        }
                    }
                    variant_index++;
                }
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
            if (value_name == "Variant") {
                entry.lizard_variant = from_jv<int32>(*value_jv_ptr);
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
        database.ability_entries[ability_index(entry.lizard_type, entry.lizard_variant, entry.ability_type)] = entry;
    }
    for (uint32 i = 0; i < magic_enum::enum_count<LizardType>(); i++) {
        for (uint8 v = 0; v < LIZARD_VARIANT_COUNT; v++) {
            auto& attack_entry = database.ability_entries[ability_index(LizardType(i), v, AbilityType_Attack)];
            auto& spell_entry = database.ability_entries[ability_index(LizardType(i), v, AbilityType_Spell)];

            attack_entry.extra_strings["Spell"] = spell_entry.name;
            spell_entry.extra_strings["Attack"] = attack_entry.name;
        }
    }
    database.initialized = true;
}

entt::entity instance_lizard(Scene* scene, v3i position, LizardType lizard_type, uint8 lizard_variant, std::function<void(Scene*, entt::entity)> build_func) {
    auto& lizard_entry = get_lizard_database().lizard_entries.at(lizard_type);

    auto entity = scene->registry.create();
    scene->registry.emplace<AddToInspect>(entity);

    auto& model_comp = scene->registry.emplace<Model>(entity);
    model_comp.model_cpu = std::make_unique<ModelCPU>(load_resource<ModelCPU>(lizard_entry.model_path));
    for (id_ptr<ModelCPU::Node>& node : model_comp.model_cpu->nodes) {
        if (node->mesh_path_options.size() > 1)
            node->mesh_selection = lizard_entry.variants[lizard_variant].uv;
    }
    model_comp.model_gpu = instance_model(scene->render_scene, *model_comp.model_cpu);

    scene->registry.emplace<LogicTransform>(entity, v3(position));
    scene->registry.emplace<Grounded>(entity);
    scene->registry.emplace<ModelTransform>(entity);
    scene->registry.emplace<TransformLink>(entity, v3(0.5f, 0.5f, 0.0f));

    scene->registry.emplace<EmitterComponent>(entity, scene);
    scene->registry.emplace<Health>(entity, 10.0f, scene, entity, lizard_entry.hurt_emitter);
    scene->registry.emplace<Tags>(entity, *scene);

    scene->registry.emplace<Text3D>(entity, scene);
    scene->registry.emplace<Text3DTransform>(entity);

    if (model_comp.model_cpu->skeleton) {
        scene->registry.emplace<PoseController>(entity, *model_comp.model_cpu->skeleton);
    }

    scene->registry.emplace<Name>(entity, fmt_("{},{}", lizard_entry.name, lizard_entry.variants.at(lizard_variant).title));

    scene->registry.emplace<Caster>(entity, scene, entity);
    scene->registry.emplace<Draggable>(entity, 0.5f);
    scene->registry.emplace<Team>(entity, Team::TeamID_Lizards);

    ModelTransform& model_tfm = scene->registry.get<ModelTransform>(entity);
    model_tfm.set_scale(v3(lizard_entry.scale));

    PoseController* poser = scene->registry.try_get<PoseController>(entity);
    if (poser)
        poser->set_state(AnimationState_Idle, 0.0f);
    Health& health = scene->registry.get<Health>(entity);
    health.regen = std::make_unique<Stat>(scene, entity, 0.0f);

    build_func(scene, entity);

    return entity;
}

}