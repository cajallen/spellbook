#pragma once

#include <entt/entity/entity.hpp>

namespace spellbook {

struct Scene;
struct LizardPrefab;

void build_assassin(Scene* scene, entt::entity entity, const LizardPrefab& lizard_prefab);
void build_barbarian(Scene* scene, entt::entity entity, const LizardPrefab& lizard_prefab);
void build_champion(Scene* scene, entt::entity entity, const LizardPrefab& lizard_prefab);
void build_ranger(Scene* scene, entt::entity entity, const LizardPrefab& lizard_prefab);
void build_warlock(Scene* scene, entt::entity entity, const LizardPrefab& lizard_prefab);
void build_illusion_mage(Scene* scene, entt::entity entity, const LizardPrefab& lizard_prefab);
void build_mind_mage(Scene* scene, entt::entity entity, const LizardPrefab& lizard_prefab);
void build_water_mage(Scene* scene, entt::entity entity, const LizardPrefab& lizard_prefab);

void draw_champion_dragging_preview(Scene* scene, entt::entity entity);
void draw_warlock_dragging_preview(Scene* scene, entt::entity entity);
void draw_assassin_dragging_preview(Scene* scene, entt::entity entity);
void draw_ranger_dragging_preview(Scene* scene, entt::entity entity);

}
