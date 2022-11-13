#include "asset_editor.hpp"

#include <tracy/Tracy.hpp>

#include "asset_browser.hpp"
#include "game.hpp"

#include "lib_ext/imgui_extra.hpp"

#include "input.hpp"

#include "game/components.hpp"
#include "lib_ext/icons/font_awesome4.h"
#include "renderer/draw_functions.hpp"
#include "vuk/Partials.hpp"

namespace spellbook {

void AssetEditor::setup() {
    ZoneScoped;
    p_scene = new Scene();
    p_scene->setup("Asset Editor");

    {
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("src/shaders/particle_emitter.comp"), "particle_emitter.comp");
        game.renderer.context->create_named_pipeline("emitter", pci);
    }
    {
        vuk::PipelineBaseCreateInfo pci2;
        pci2.add_glsl(get_contents("src/shaders/particle.vert"), "particle.vert");
        pci2.add_glsl(get_contents("src/shaders/textured_3d.frag"), "textured_3d.frag");
        game.renderer.context->create_named_pipeline("particle", pci2);
    }
    
    emitter.settings = {
        v4(-0.04f,-0.04f, -0.04f,0.05f),
        v4(-1.0f,-1.0f,-0.25f,0.997f),
        v4(0.75f,0.5f, 0.75f,0.0f),
        v4(0.08f,0.08f,0.08f,0.1f),
        v4(0.2f,0.2f,0.2f,0.001f),
        0.5,
        1.5
    };
    emitter.rate = 0.01f;
    emitter.calculate_max();
    emitter.next_spawn = Input::time;
    emitter.mesh = game.renderer.upload_mesh(generate_icosphere(2));
    MaterialCPU material_cpu = {
        .file_path = "particle_mat",
        .color_tint = palette::gray_5,
        .shader_name = "particle"
    };
    emitter.material = game.renderer.upload_material(material_cpu);

    struct Particle {
        v4 position_scale;
        v4 velocity_damping;
        v4 color;
        v4 life;
    };
    vector<u8> bytes;
    bytes.resize(emitter.settings.max_particles * sizeof(Particle) + 1);
    auto [buf, fut] = create_buffer_gpu(*game.renderer.global_allocator, vuk::DomainFlagBits::eTransferOnTransfer, std::span(bytes));
    emitter.particles_buffer = std::move(buf);
    game.renderer.enqueue_setup(std::move(fut));
    
    p_scene->render_scene.emitters.emplace_back(std::move(emitter));
}

void AssetEditor::update() {
    ZoneScoped;

    static bool ticker = false;
    auto& emitter_settings = p_scene->render_scene.emitters.last().settings;
    emitter_settings.velocity_damping = v4(4.0f * math::euler2vector(euler{Input::time * 3.0f + (ticker ? math::PI : 0.0f), 0.0f}), 0.998f) - emitter_settings.velocity_damping_random * 0.5f;
    ticker = !ticker;
    
    if (Input::mouse_click[GLFW_MOUSE_BUTTON_LEFT]) {
        p_scene->render_scene.query = v2i(Input::mouse_pos) - p_scene->render_scene.viewport.start;
    }
}

void AssetEditor::window(bool* p_open) {
    ZoneScoped;

    if (ImGui::Begin("Asset Editor", p_open)) {
        if (ImGui::BeginTabBar("Asset Types")) {
            if (ImGui::BeginTabItem("Material")) {
                tab = Tab_Material;
                inspect(&material_cpu);
                
                if (ImGui::Button("Save")) {
                    file_dump(from_jv<json>(to_jv(material_cpu)), material_cpu.file_path);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    string backup_path = material_cpu.file_path;
                    material_cpu = from_jv<MaterialCPU>(to_jv(parse_file(material_cpu.file_path)));
                    material_cpu.file_path = backup_path;
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Model")) {
                tab = Tab_Model;
                inspect(&model_cpu);
                
                if (ImGui::Button("Save")) {
                    save_model(model_cpu);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    model_cpu = load_model(model_cpu.file_path);
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Tower")) {
                tab = Tab_Tower;
                inspect(&tower_prefab);
                
                if (ImGui::Button("Save")) {
                    save_tower(tower_prefab);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    tower_prefab = load_tower(tower_prefab.file_path);
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Tile")) {
                tab = Tab_Tile;
                inspect(&tile_prefab);
                
                if (ImGui::Button("Save")) {
                    save_tile(tile_prefab);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    tile_prefab = load_tile(tile_prefab.file_path);
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Enemy")) {
                tab = Tab_Enemy;
                inspect(&enemy_prefab);
                
                if (ImGui::Button("Save")) {
                    save_enemy(enemy_prefab);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    enemy_prefab = load_enemy(enemy_prefab.file_path);
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Spawner")) {
                tab = Tab_Spawner;
                inspect(&spawner_prefab);
                
                if (ImGui::Button("Save")) {
                    save_spawner(spawner_prefab);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    spawner_prefab = load_spawner(spawner_prefab.file_path);
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Consumer")) {
                tab = Tab_Consumer;
                inspect(&consumer_prefab);
                
                if (ImGui::Button("Save")) {
                    save_consumer(consumer_prefab);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    consumer_prefab = load_consumer(consumer_prefab.file_path);
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();

    // static auto render_scene = new RenderScene();
    // static bool initialized = false;
    //
    // if (!initialized && !model_comp.model_cpu.file_name.empty()) {
    //     render_scene->name = "model_thumbnail";
    //     render_scene->viewport.name	 = render_scene->name + "::viewport";
    //     v3 cam_position = 4.f * v3(1, 1, 1);
    //     render_scene->viewport.camera = new Camera(cam_position, math::vector2euler(-cam_position));
    //     render_scene->viewport.setup();
    //
    //     ModelGPU model_gpu = instance_model(*render_scene, model_comp.model_cpu);
    //     
    //     game.renderer.add_scene(render_scene);
    //     initialized = true;
    // }
}


}
