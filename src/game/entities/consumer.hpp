#pragma once

#include <entt/entity/entity.hpp>

#include "general/math/geometry.hpp"
#include "general/file/resource.hpp"

namespace spellbook {

struct ConsumerPrefab : Resource {
    FilePath shrine_model_path;
    FilePath egg_model_path;

    static constexpr string_view extension() { return ".sbjcon"; }
    static constexpr string_view dnd_key() { return "DND_CONSUMER"; }
    static FilePath folder() { return "consumers"_resource; }
    static std::function<bool(const FilePath&)> path_filter() { return [](const FilePath& path) { return path.extension() == ConsumerPrefab::extension(); }; }
};

JSON_IMPL(ConsumerPrefab, shrine_model_path, egg_model_path);

struct Scene;
entt::entity instance_prefab(Scene*, const ConsumerPrefab&, v3i location);
bool inspect(ConsumerPrefab*);

void consumer_system(Scene* scene);

struct Shrine {
    entt::entity egg_entity;
    bool egg_attached;
};

struct Egg {};

}