#include "game/shop.hpp"

#include <imgui.h>

#include "game.hpp"
#include "extension/fmt.hpp"
#include "extension/imgui_extra.hpp"
#include "game/player.hpp"
#include "game/scene.hpp"
#include "game/entities/drop.hpp"
#include "game/entities/components.hpp"
#include "game/entities/spawner.hpp"

namespace spellbook {

ShopEntry* Warehouse::get_entry() {
    assert_else(entries.size() == probabilities.size() && probabilities.size() == stock.size())
        return &entries.front();

    float total_probability = 0.0f;
    for (uint32 i = 0; i < entries.size(); i++) {
        total_probability += stock[i] ? probabilities[i] : 0.0f;
    }

    float current_probability = 0.0f;
    float target_probability = math::random_float();
    for (uint32 i = 0; i < entries.size(); i++) {
        float entry_probability = stock[i] ? probabilities[i] / total_probability : 0.0f;
        current_probability += entry_probability;
        if (target_probability <= current_probability) {
            stock[i] -= 1;
            return &entries[i];
        }
    }
    
    return nullptr;
}

void Warehouse::add_entry(ShopEntry&& shop_entry, float probability) {
    entries.push_back(shop_entry);
    probabilities.push_back(probability);
    stock.push_back(1);
}



void ShopGenerator::setup(Scene* scene) {
    log_warning("Attempted to use unenforced virtual ShopGenerator");
}
vector<ShopEntry*>* ShopGenerator::generate_shop() {
    log_warning("Attempted to use unenforced virtual ShopGenerator");
    return nullptr;
}
void ShopGenerator::purchase(uint32 index) {
    log_warning("Attempted to use unenforced virtual ShopGenerator");
}
void ShopGenerator::reset() {
    log_warning("Attempted to use unenforced virtual ShopGenerator");
}
void ShopGenerator::inspect() {
    log_warning("Attempted to use unenforced virtual ShopGenerator");
}

void FirstFreeShopGenerator::setup(Scene* scene) {
    round_info = scene->spawn_state_info;
    first_warehouse.add_entry({Bead_Oak, 0, "lizards/champion.sbliz"}, 1.0f);
    // first_warehouse.add_entry({Bead_Oak, 0, "lizards/warlock.sbliz"}, 1.0f);
    first_warehouse.add_entry({Bead_Oak, 0, "lizards/assassin.sbliz"}, 1.0f);
    first_warehouse.add_entry({Bead_Oak, 0, "lizards/ranger.sbliz"}, 1.0f);

    warehouse.add_entry({Bead_Oak, 3, "lizards/champion.sbliz"}, 1.0f);
    warehouse.add_entry({Bead_Oak, 5, "lizards/warlock.sbliz"}, 1.0f);
    warehouse.add_entry({Bead_Oak, 4, "lizards/assassin.sbliz"}, 1.0f);

    warehouse.add_entry({Bead_Malachite, 3, "lizards/champion.sbliz"}, 1.0f);
    warehouse.add_entry({Bead_Malachite, 1, "lizards/warlock.sbliz"}, 1.0f);
    warehouse.add_entry({Bead_Malachite, 2, "lizards/assassin.sbliz"}, 1.0f);
}
vector<ShopEntry*>* FirstFreeShopGenerator::generate_shop() {
    assert_else(out_shop.empty())
        return &out_shop;

    for (uint32 i = 0; i < shop_size; i++) {
        auto entry = round_info->round_number >= 0 ? warehouse.get_entry() : first_warehouse.get_entry();
        if (entry == nullptr)
            break;
        out_shop.push_back(entry);
    }
    return &out_shop;
}
void FirstFreeShopGenerator::purchase(uint32 index) {
    if (round_info->round_number == -1) {
        reset();
        round_info->advance_round();
    } else {
        out_shop.remove_index(index);
    }
}

void FirstFreeShopGenerator::reset() {
    if (round_info->round_number >= 0) {
        for (uint32 i = 0; i < out_shop.size(); i++) {
            uint32 j = warehouse.entries.index(*out_shop[i]);
            warehouse.stock[j]++;
        }
    }
    out_shop.clear();
}
void FirstFreeShopGenerator::inspect() {
    ImGui::InputInt("Shop Size", &shop_size);
    ImGui::Text("Warehouse");
    ImGui::Indent();
    spellbook::inspect(&warehouse);
    ImGui::Unindent();
}


void SimpleShopGenerator::setup(Scene* scene) {
    round_info = scene->spawn_state_info;
    warehouse.add_entry({Bead_Oak, 4, "lizards/zord.sbliz"}, 1.0f);
    warehouse.add_entry({Bead_Oak, 6, "lizards/rokko.sbliz"}, 1.0f);
    warehouse.add_entry({Bead_Oak, 6, "lizards/merque.sbliz"}, 1.0f);
    warehouse.add_entry({Bead_Oak, 8, "lizards/azura.sbliz"}, 1.0f);
    warehouse.add_entry({Bead_Oak, 8, "lizards/sauria.sbliz"}, 1.0f);
    warehouse.add_entry({Bead_Oak, 6, "lizards/rafaj.sbliz"}, 1.0f);

    warehouse.add_entry({Bead_Malachite, 4, "lizards/rokko.sbliz"}, 1.0f);
    warehouse.add_entry({Bead_Malachite, 3, "lizards/merque.sbliz"}, 1.0f);
    warehouse.add_entry({Bead_Malachite, 3, "lizards/azura.sbliz"}, 1.0f);
    warehouse.add_entry({Bead_Malachite, 3, "lizards/sauria.sbliz"}, 1.0f);
    warehouse.add_entry({Bead_Malachite, 4, "lizards/rafaj.sbliz"}, 1.0f);
}
vector<ShopEntry*>* SimpleShopGenerator::generate_shop() {
    assert_else(out_shop.empty())
        return &out_shop;

    for (uint32 i = 0; i < shop_size; i++) {
        auto entry = warehouse.get_entry();
        if (entry == nullptr)
            break;
        out_shop.push_back(entry);
    }
    return &out_shop;
}
void SimpleShopGenerator::purchase(uint32 index) {
    out_shop.remove_index(index);
}

void SimpleShopGenerator::reset() {
    for (uint32 i = 0; i < out_shop.size(); i++) {
        uint32 j = warehouse.entries.index(*out_shop[i]);
        warehouse.stock[j]++;
    }
    out_shop.clear();
}
void SimpleShopGenerator::inspect() {
    ImGui::InputInt("Shop Size", &shop_size);
    ImGui::Text("Warehouse");
    ImGui::Indent();
    spellbook::inspect(&warehouse);
    ImGui::Unindent();
}

bool button(ShopEntry* shop_entry) {
    string button_text = shop_entry->cost_amount > 0 ? fmt_("{}\n{} {}{}",
        shop_entry->lizard_prefab_path,
        shop_entry->cost_amount,
        magic_enum::enum_name(shop_entry->cost_type),
        shop_entry->cost_amount > 1 ? "s" : ""
    ) : fmt_("{}\n{}", shop_entry->lizard_prefab_path, "Free");
    return ImGui::Button(button_text.c_str(), {200, 80});
}

void inspect(ShopEntry* shop_entry) {
    ImGui::EnumCombo("Cost Type", &shop_entry->cost_type);
    ImGui::InputInt("Cost Amount", &shop_entry->cost_amount);
    ImGui::PathSelect("Lizard Path", &shop_entry->lizard_prefab_path, FileType_Lizard);
}
void inspect(Warehouse* warehouse) {
    if (ImGui::BeginTable("Warehouse", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Entry");
        ImGui::TableSetupColumn("Probability");
        ImGui::TableSetupColumn("Stock");
        ImGui::TableHeadersRow();
        for (uint32 i = 0; i < warehouse->entries.size(); i++) {
            ImGui::PushID(&warehouse->entries[i]);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            inspect(&warehouse->entries[i]);
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::DragFloat("##Probability", &warehouse->probabilities[i], 0.01f);
            ImGui::TableSetColumnIndex(2);
            bool stock_as_bool = warehouse->stock[i];
            ImGui::Checkbox("##Stock", &stock_as_bool);
            warehouse->stock[i] = stock_as_bool ? 1 : 0;
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}
void show_shop(Shop* shop, Player* player) {
    ImGui::BeginGroup();
    for (uint32 i = 0; i < player->bank.beads.size(); i++) {
        auto bead_name = string(magic_enum::enum_name((Bead) i));
        ImGui::SetNextItemWidth(70.0f);
        ImGui::InputInt(bead_name.c_str(), &player->bank.beads[i]);
    }
    ImGui::EndGroup();

    ImGui::SameLine();
    
    ImGui::BeginGroup();
    if (ImGui::Button("Get Shop")) {
        player->scene->audio.play_sound("audio/reroll.flac", {.global = true, .volume = 0.3f});
        shop->shop_generator->reset();
        shop->entries = shop->shop_generator->generate_shop();
    }
    ImGui::SameLine();
    if (ImGui::Button("Close")) {
        shop->shop_generator->reset();
        shop->shop_generator->round_info->advance_round();
    }
    if (shop->entries != nullptr) {
        for (uint32 i = 0; i < shop->entries->size(); i++) {
            if (i > 0)
                ImGui::SameLine();
            Bead bead_type = shop->entries->at(i)->cost_type;
            Color color = shop->entries->at(i)->cost_amount > 0 ? bead_color(bead_type) : palette::gray_7;
            
            ImGui::BeginDisabled(player->bank.beads[bead_type] < shop->entries->at(i)->cost_amount);
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4) Color(color, 0.4f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4) Color(color, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4) Color(color.rgb * 0.8f, 1.0f));
            if (button(shop->entries->at(i))) {
            }
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                if (player->scene->selected_entity == entt::null) {
                    v3  intersect = math::intersect_axis_plane(player->scene->render_scene.viewport.ray((v2i) Input::mouse_pos), Z, 0.0f);
                    
                    entt::entity lizard = instance_prefab(
                        player->scene,
                        load_asset<LizardPrefab>(shop->entries->at(i)->lizard_prefab_path, true),
                        math::round_cast(intersect)
                    );
                    player->scene->audio.play_sound("audio/drop.flac", {.global = true, .volume = 0.3f});
                    player->scene->select_entity(lizard);
                    player->bank.beads[bead_type] -= shop->entries->at(i)->cost_amount;
                    shop->shop_generator->purchase(i);
                }
                ImGui::Text("Place");
                ImGui::EndDragDropSource();
            }
            ImGui::PopStyleColor(3);
            ImGui::EndDisabled();
        }
    }
    ImGui::EndGroup();
}


}
