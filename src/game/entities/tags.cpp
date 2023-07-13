#include "tags.hpp"

#include "game/scene.hpp"

namespace spellbook {

bool Tags::has_tag(uint32 tag) {
    for (auto it = entries.begin(); it != entries.end();) {
        Entry& entry = it->second;
        if (entry.until < scene.time) {
            it = entries.erase(it);
            continue;
        }
        if (entry.has_dependency && scene.registry.valid(entry.dependency)) {
            it = entries.erase(it);
            continue;
        }

        if (tag == entry.tag)
            return true;
    }
    return false;
}

void Tags::remove_tag(uint64 id) {
    if (entries.contains(id))
        entries.erase(id);
}


void Tags::apply_tag(uint32 tag, uint64 id, float duration) {
    if (entries.contains(id)) {
        if (entries[id].until > scene.time + duration)
            return;
    }
    entries[id] = {tag, scene.time + duration};
}

void Tags::apply_tag(uint32 tag, uint64 id, entt::entity dependency, float duration) {
    if (entries.contains(id)) {
        if (entries[id].until > scene.time + duration)
            return;
    }
    entries[id] = {tag, scene.time + duration, true, dependency};
}

}