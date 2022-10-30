#include "test_scene.hpp"

#include "game.hpp"
#include "spline.hpp"
#include "renderer/draw_functions.hpp"

namespace spellbook {

void TestScene::setup() {
    setup(new Scene());
}

void TestScene::setup(Scene* init_scene) {
    p_scene = init_scene;
}

void TestScene::update() {
    Viewport& viewport = p_scene->render_scene.viewport;

    vector<FormattedVertex> formatted_vertices;
    for (v3& p : control_points) {
        formatted_vertices.emplace_back(p, palette::spellbook_2, 0.03f);
    }
    if (control_points.size() >= 2) {
        auto line_mesh = generate_formatted_line(viewport.camera, formatted_vertices);
        Renderable control_line;
        control_line.mesh_asset_path = game.renderer.upload_mesh(line_mesh);
        control_line.material_asset_path = "default";
        control_line.frame_allocated = true;

        p_scene->render_scene.add_renderable(control_line);
    }

    vector<FormattedVertex> formatted_vertices2;

    for (u32 i = 0; i <= 100; i++) {
        v3 p = bspline_v2(f32(i) / 100.f, control_points);
        formatted_vertices2.emplace_back(p, palette::spellbook_0, 0.03f);
    }
    if (formatted_vertices2.size() > 2) {
        auto line_mesh = generate_formatted_line(viewport.camera, formatted_vertices2);
        Renderable t_curve;
        t_curve.mesh_asset_path = game.renderer.upload_mesh(line_mesh);
        t_curve.material_asset_path = "default";
        t_curve.frame_allocated = true;
        
        p_scene->render_scene.add_renderable(t_curve);
    }
    // auto line_mesh = generate_formatted_line(viewport.camera, {
    //         {p + v3(0.f,0.f,0.03f), palette::spellbook_1, 0.04f},
    //         {p - v3(0.f,0.f,0.03f), palette::spellbook_1, 0.04f}
    // });
    // Renderable t_dot;
    // t_dot.mesh_asset_path = game.renderer.upload_mesh(line_mesh);
    // t_dot.material_asset_path = "default";
    // t_dot.frame_allocated = true;
    //
    // p_scene->render_scene.add_renderable(t_dot);

}

void TestScene::window(bool* p_open) {
    if (ImGui::Begin("Test Scene", p_open)) {
        ImGui::Text("Control Points");
        for (v3& p : control_points) {
            ImGui::PushID(&p);
            ImGui::DragFloat3("Position", p.data, 0.01f);
            ImGui::SameLine();
            ImGui::Dummy(ImVec2{ImGui::GetContentRegionAvail().x - 20.f, 0});
            ImGui::SameLine();
            if (ImGui::SmallButton("x")) {
                control_points.remove_index(control_points.index(p));
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }
        if (ImGui::Button("Add"))
            control_points.emplace_back();
        ImGui::Separator();
        ImGui::SliderInt("degrees", &degrees, 1, 6);

        ImGui::DragFloat("t", &t, 0.01f);
    }
    ImGui::End();
}

}
