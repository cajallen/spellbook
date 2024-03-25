#pragma once

#include "general/memory.hpp"
#include "general/color.hpp"
#include "general/math/geometry.hpp"
#include "general/file/file_path.hpp"

#define HIERARCHY_DEPTH_INCREMENT 2
#define DISTORTION_AMOUNT 5.0f
#define DISTORTION_TIME Input::time * 5.0f

namespace spellbook {

struct GUIManager;

struct ItemGUI {
    GUIManager& manager;
    range2i given_region;
    int32 depth;

    bool hoverable;
    bool clickable; // requires hoverable

    ItemGUI(GUIManager& manager);
    virtual void draw();
    virtual range2i get_occupied_region();
    void add_interact_region(range2i region);
    uint64 get_id();
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
    bool tiling;
 public:

    Color tint = palette::white;

    using ItemGUI::ItemGUI;

    void draw() override;
    range2i get_occupied_region() override;
    void set_image(const FilePath& fp, v2i bounds, bool keep_aspect = true);
    void set_image_tiling(const FilePath& fp, float scale);
};

struct PanelGUI : ItemGUI {
    v2i start_padding;
    v2i end_padding;
    unique_ptr<ItemGUI> item;
    FilePath texture_path = "white"_symbolic;
    Color background_tint = palette::white;
    Color outline_color = palette::black;
    float outline_width = 2.0f;
    bool show = true;
    bool shrink = true;

    using ItemGUI::ItemGUI;

    void draw() override;
    range2i get_occupied_region() override;
};

struct AlignmentGUI : ItemGUI {
    v2 pos; // 0-1
    unique_ptr<ItemGUI> item;

    using ItemGUI::ItemGUI;

    void draw() override;
    range2i get_occupied_region() override;
};

struct VerticalSeparator : ItemGUI {
    uint8 height = 8;
    Color color = palette::black;
    float width = 2.0f;

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
    float background_opacity = 0.0f;

    using ItemGUI::ItemGUI;

    void draw() override;
    range2i get_occupied_region() override;
};

struct HorizontalBox : ItemGUI {
    vector<unique_ptr<ItemGUI>> items;
    vector<float> spacing_weights;
    float background_opacity = 0.0f;

    using ItemGUI::ItemGUI;

    void draw() override;
    range2i get_occupied_region() override;
};

struct StackGUI : ItemGUI {
    vector<unique_ptr<ItemGUI>> items;

    using ItemGUI::ItemGUI;

    void draw() override;
    range2i get_occupied_region() override;
};

}