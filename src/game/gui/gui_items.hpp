#pragma once

#include "general/memory.hpp"
#include "general/color.hpp"
#include "general/math/geometry.hpp"
#include "general/file/file_path.hpp"

namespace spellbook {

struct GUIManager;

struct ItemGUI {
    GUIManager& manager;
    range2i given_region;
    int32 depth;

    ItemGUI(GUIManager& manager);
    virtual void draw();
    virtual range2i get_occupied_region();
};

struct SpacingGUI : ItemGUI {
    v2i size;

    using ItemGUI::ItemGUI;

    range2i get_occupied_region() override;
};

struct ButtonGUI : ItemGUI {
    unique_ptr<ItemGUI> item;

    using ItemGUI::ItemGUI;

    void draw() override;
    range2i get_occupied_region() override;
};

struct ImageGUI : ItemGUI {
 protected:
    FilePath texture_path = "white"_symbolic;
    v2i size;
 public:
    using ItemGUI::ItemGUI;

    void draw() override;
    range2i get_occupied_region() override;
    void set_image(const FilePath& fp, v2i bounds, bool keep_aspect = true);
};

struct Panel : ItemGUI {
    v2i padding;
    unique_ptr<ItemGUI> item;
    FilePath texture_path = "white"_symbolic;
    Color background_tint = palette::white;

    using ItemGUI::ItemGUI;

    void draw() override;
    range2i get_occupied_region() override;
};

struct TextGUI : ItemGUI {
    string text;

    umap<string, float>* ref_floats;
    umap<string, string>* ref_strings;

    using ItemGUI::ItemGUI;

    void draw() override;
    range2i get_occupied_region() override;
};

struct VerticalBox : ItemGUI {
    vector<unique_ptr<ItemGUI>> items;
    vector<float> spacing_weights;

    using ItemGUI::ItemGUI;

    void draw() override;
    range2i get_occupied_region() override;
};

struct HorizontalBox : ItemGUI {
    vector<unique_ptr<ItemGUI>> items;
    vector<float> spacing_weights;

    using ItemGUI::ItemGUI;

    void draw() override;
    range2i get_occupied_region() override;
};

}