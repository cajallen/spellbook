#include "game/shop.hpp"

#include <imgui.h>

#include "components.hpp"
#include "extension/fmt.hpp"
#include "extension/imgui_extra.hpp"
#include "game/player.hpp"

namespace spellbook {

ShopEntry* Warehouse::get_entry() {
    assert_else(entries.size() == probabilities.size() && probabilities.size() == stock.size())
        return &entries.front();

    float total_probability = 0.0f;
    for (u32 i = 0; i < entries.size(); i++) {
        total_probability += stock[i] ? probabilities[i] : 0.0f;
    }

    float current_probability = 0.0f;
    float target_probability = math::random_f32();
    for (u32 i = 0; i < entries.size(); i++) {
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



void ShopGenerator::setup() {
    log_warning("Attempted to use unenforced virtual ShopGenerator");
}
vector<ShopEntry*>* ShopGenerator::generate_shop() {
    log_warning("Attempted to use unenforced virtual ShopGenerator");
    return nullptr;
}
void ShopGenerator::purchase(u32 index) {
    log_warning("Attempted to use unenforced virtual ShopGenerator");
}
void ShopGenerator::reset() {
    log_warning("Attempted to use unenforced virtual ShopGenerator");
}
void ShopGenerator::inspect() {
    log_warning("Attempted to use unenforced virtual ShopGenerator");
}


void SimpleShopGenerator::setup() {
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

    for (u32 i = 0; i < shop_size; i++) {
        auto entry = warehouse.get_entry();
        if (entry == nullptr)
            break;
        out_shop.push_back(entry);
    }
    return &out_shop;
}
void SimpleShopGenerator::purchase(u32 index) {
    out_shop.remove_index(index);
}

void SimpleShopGenerator::reset() {
    for (u32 i = 0; i < out_shop.size(); i++) {
        u32 j = warehouse.entries.index(*out_shop[i]);
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
    string button_text = fmt_("{}\n{} {}{}",
        shop_entry->lizard_prefab_path,
        shop_entry->cost_amount,
        magic_enum::enum_name(shop_entry->cost_type),
        shop_entry->cost_amount > 1 ? "s" : ""
    );
    return ImGui::Button(button_text.c_str());
}


entt::entity instance_prefab(Scene* scene, const BeadPrefab& bead_prefab, v3 position) {
    static int i      = 0;
    auto       entity = scene->registry.create();
    scene->registry.emplace<Name>(entity, fmt_("{}_{}", fs::path(bead_prefab.file_path).stem().string(), i++));
    
    auto& model_comp = scene->registry.emplace<Model>(entity);
    model_comp.model_cpu = std::make_unique<ModelCPU>(load_asset<ModelCPU>(bead_prefab.model_path));
    model_comp.model_gpu = std::move(instance_model(scene->render_scene, *model_comp.model_cpu));

    scene->registry.emplace<LogicTransform>(entity, position);
    scene->registry.emplace<ModelTransform>(entity);
    scene->registry.emplace<TransformLink>(entity, v3(0.5));

    scene->registry.emplace<Pickup>(entity, bead_prefab.type);
    
    return entity;  
}

bool inspect(BeadPrefab* bead_prefab) {
    bool changed = false;
    ImGui::PathSelect("File", &bead_prefab->file_path, "resources/drops", FileType_Drop, true);
    changed |= ImGui::EnumCombo("Type", &bead_prefab->type);
    changed |= ImGui::PathSelect("Model", &bead_prefab->model_path, "resources/models", FileType_Model, true);
    return changed;
}

void inspect(ShopEntry* shop_entry) {
    ImGui::EnumCombo("Cost Type", &shop_entry->cost_type);
    ImGui::InputInt("Cost Amount", &shop_entry->cost_amount);
    ImGui::PathSelect("Lizard Path", &shop_entry->lizard_prefab_path, "resources/lizards", FileType_Lizard);
}
void inspect(Warehouse* warehouse) {
    if (ImGui::BeginTable("Warehouse", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Entry");
        ImGui::TableSetupColumn("Probability");
        ImGui::TableSetupColumn("Stock");
        ImGui::TableHeadersRow();
        for (u32 i = 0; i < warehouse->entries.size(); i++) {
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
    for (u32 i = 0; i < player->bank.beads.size(); i++) {
        auto bead_name = string(magic_enum::enum_name((Bead) i));
        ImGui::SetNextItemWidth(65.0f);
        ImGui::InputInt(bead_name.c_str(), &player->bank.beads[i]);
    }
    ImGui::EndGroup();

    ImGui::SameLine();
    
    ImGui::BeginGroup();
    if (ImGui::Button("Get Shop")) {
        shop->shop_generator->reset();
        shop->entries = shop->shop_generator->generate_shop();
    }
    if (shop->entries != nullptr) {
        for (u32 i = 0; i < shop->entries->size(); i++) {
            if (i > 0)
                ImGui::SameLine();
            auto bead_type = shop->entries->at(i)->cost_type;
            ImGui::BeginDisabled(player->bank.beads[bead_type] < shop->entries->at(i)->cost_amount);
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4) Color(bead_color(bead_type), 0.4f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4) Color(bead_color(bead_type), 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4) Color(bead_color(bead_type).rgb * 0.8f, 1.0f));
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

Color bead_color(Bead bead) {
    switch (bead) {
        case (Bead_Oak): {
            return Color(0.82f, 0.66f, 0.37f);
        } break;
        case (Bead_Yew): {
            return Color(0.65f, 0.15f, 0.20f);
        } break;
        case (Bead_Amber): {
            return Color(0.87f, 0.37f, 0.02f);
        } break;
        case (Bead_Malachite): {
            return Color(0.24f, 0.80f, 0.60f);
        } break;
    }
    log_warning("Missing bead color");
    return palette::black;
}


}
