#pragma once

#include "general/string.hpp"
#include "general/vector.hpp"
#include "game/lizard.hpp"

namespace spellbook {

struct Scene;
struct Player;

enum Bead {
    Bead_Oak,
    Bead_Yew,
    Bead_Amber,
    Bead_Malachite,
    Bead_Count
};

struct BeadPrefab {
    string file_path;
    string model_path;
    Bead type;
};

struct ShopEntry {
    Bead cost_type;
    int cost_amount;
    string lizard_prefab_path;
};

struct Warehouse {
    vector<ShopEntry> entries;
    vector<float> probabilities;
    vector<u8> stock;

    ShopEntry* get_entry();
    void add_entry(ShopEntry&& shop_entry, float probability);
};

struct ShopGenerator {
    virtual void setup();
    virtual vector<ShopEntry*>* generate_shop();
    virtual void purchase(u32 index);
    virtual void reset();
    virtual void inspect();
};

struct SimpleShopGenerator : ShopGenerator {
    int shop_size = 3;
    Warehouse warehouse;
    vector<ShopEntry*> out_shop;
    
    void setup() override;
    vector<ShopEntry*>* generate_shop() override;
    void purchase(u32 index) override;
    void reset() override;
    void inspect() override;
};

struct Shop {
    std::unique_ptr<ShopGenerator> shop_generator;
    vector<ShopEntry*>* entries;
};

bool button(ShopEntry* shop_entry);
void show_shop(Shop* shop, Player* player);

entt::entity instance_prefab(Scene* scene, const BeadPrefab& bead_prefab, v3 position);

void inspect(ShopEntry* shop_entry);
void inspect(Warehouse* warehouse);
bool inspect(BeadPrefab* bead_prefab);
//void inspect(Shop* shop);

Color bead_color(Bead bead);

JSON_IMPL(BeadPrefab, model_path, type);

}