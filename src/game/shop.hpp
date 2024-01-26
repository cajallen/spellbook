#pragma once

#include "general/string.hpp"
#include "general/vector.hpp"
#include "game/entities/drop.hpp"
#include "game/entities/lizard.hpp"

namespace spellbook {

struct Scene;
struct Player;
struct SpawnStateInfo;
struct Item;

struct ShopEntry {
    Bead cost_type = Bead_Oak;
    int cost_amount = 0;
    FilePath lizard_prefab_path = {};
    Item* item = nullptr;
};

struct Warehouse {
    vector<ShopEntry> entries = {};
    vector<float> probabilities = {};
    vector<uint8> stock = {};

    ShopEntry* get_entry();
    void add_entry(ShopEntry&& shop_entry, float probability);
};

struct ShopGenerator {
    SpawnStateInfo* round_info = nullptr;
    
    virtual void setup(Scene* scene);
    virtual vector<ShopEntry*>* generate_shop();
    virtual void purchase(uint32 index);
    virtual void reset();
    virtual void inspect();
};

struct SimpleShopGenerator : ShopGenerator {
    int shop_size = 3;
    Warehouse warehouse = {};
    vector<ShopEntry*> out_shop;
    
    void setup(Scene* scene) override;
    vector<ShopEntry*>* generate_shop() override;
    void purchase(uint32 index) override;
    void reset() override;
    void inspect() override;
};

struct FirstFreeShopGenerator : ShopGenerator {
    int shop_size = 3;
    Warehouse first_warehouse = {};
    Warehouse warehouse = {};
    vector<ShopEntry*> out_shop = {};
    
    void setup(Scene* scene) override;
    vector<ShopEntry*>* generate_shop() override;
    void purchase(uint32 index) override;
    void reset() override;
    void inspect() override;
};

struct Shop {
    std::unique_ptr<ShopGenerator> shop_generator = {};
    vector<ShopEntry*>* entries = nullptr;
};

bool button(ShopEntry* shop_entry);
void show_shop(Shop* shop, Player* player);

void inspect(ShopEntry* shop_entry);
void inspect(Warehouse* warehouse);
//void inspect(Shop* shop);

}