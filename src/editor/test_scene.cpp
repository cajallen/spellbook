#include "test_scene.hpp"

#include <tracy/Tracy.hpp>

#include "pose_widget.hpp"
#include "game/game.hpp"
#include "general/matrix_math.hpp"
#include "general/spline.hpp"
#include "renderer/draw_functions.hpp"

namespace spellbook {

void TestScene::setup() {
    p_scene = new Scene();
    p_scene->setup("Test Scene");
}

void TestScene::update() {
}

void TestScene::window(bool* p_open) {
    ZoneScoped;
    if (ImGui::Begin("Test Scene", p_open)) {
        
        static v3 position;
        static quat rotation;
        
        PoseWidgetState state;
        PoseWidgetSettings widget_settings{p_scene->render_scene};
        pose_widget(0, &position, &rotation, widget_settings, nullptr, &state);

        ImGui::DragFloat3("Position", position.data, 0.01f);
        ImGui::DragFloat4("Rotation", rotation.data, 0.01f);
        inspect(&state);
    }
    ImGui::End();
}

void TestScene::shutdown() {
    
}


}
