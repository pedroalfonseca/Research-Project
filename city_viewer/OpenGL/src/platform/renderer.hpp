#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "window.hpp"
#include "menu.hpp"
#include "model.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <time.h>

// --------------------------------------------------------------------------------

enum Render_Mode : ubyte {
    RENDER_MODE_GEOJSON,
    RENDER_MODE_COLLADA
};

// --------------------------------------------------------------------------------

namespace renderer {
Render_Mode       mode;

Shader            buildings_shader = {};
Shader            flat_shader      = {};
Shader            picking_shader   = {};
Shader            indices_shader   = {};
//Shader          position_shader  = {};

Model             buildings_model;
Model             flat_model;

std::vector<uint> building_indices;
std::vector<uint> tree_indices;
std::vector<uint> water_indices;

glm::mat4         projection       = {};
glm::mat4         view             = {};
glm::mat4         model            = {};

// --------------------------------------------------------------------------------

void filter_mesh_indices() {
    Mesh_Type mesh_type;

    for (size_t i = 0; i < buildings_model.meshes.size(); ++i) {
        mesh_type = buildings_model.meshes[i].type;

        if (MESH_TYPE_FLAT < mesh_type && mesh_type < MESH_TYPE_TREE) {
            water_indices.push_back(i);
        } else if (mesh_type < MESH_TYPE_BUILDING) {
            tree_indices.push_back(i);
        } else if (mesh_type < MESH_TYPE_MISC) {
            building_indices.push_back(i);
        }
    }
}

// --------------------------------------------------------------------------------

void init(Render_Mode render_mode) {
    mode = render_mode;

    // Shaders
    if (render_mode == RENDER_MODE_GEOJSON) {
        buildings_shader = make_shader("res/shaders/geojson_vert.glsl",
                                       "res/shaders/geojson_frag.glsl");
    } else {
        buildings_shader = make_shader("res/shaders/collada_vert.glsl",
                                       "res/shaders/collada_frag.glsl");
    }

    flat_shader = make_shader("res/shaders/flat_vert.glsl",
                              "res/shaders/flat_frag.glsl");

    picking_shader = make_shader("res/shaders/picking_vert.glsl",
                                 "res/shaders/picking_frag.glsl");

    indices_shader = make_shader("res/shaders/picking_vert.glsl",
                                 "res/shaders/picking_frag.glsl");

    if (render_mode == RENDER_MODE_GEOJSON) {
        // Model
        buildings_model.init("res/models/geojson/manhattan_buildings.geojson", -74.0060f, 0.0f, 40.7128f);
        flat_model.init("res/models/geojson/manhattan_ground.geojson", -74.0060f, 0.0f, 40.7128f);

        global::model_origin = buildings_model.position;

        // Camera
        camera::init(glm::vec3(2292.296143f, 50.0f, -6417.310059f));
    }

    if (render_mode == RENDER_MODE_COLLADA) {
        // Model
        //buildings_model = Model("res/models/collada/manhattan_buildings.dae", 0.0f, 0.0f, 0.0f);
        buildings_model.init("res/models/collada/manhattan.dae", 0.0f, 0.0f, 0.0f);

        global::model_origin = buildings_model.position;

        camera::init(glm::vec3(-2.97396302, 53.48173189, -2.15297508));
        //camera::init(glm::vec3(182.646774,-88.2222595, 32.0183868));
    }

    filter_mesh_indices();

    // Light
    global::light_position = camera::position;
    camera::light_position_ptr = &global::light_position;
}

void shutdown() {
    //destroy(position_shader);
    destroy(indices_shader);
    destroy(picking_shader);
    destroy(flat_shader);
    destroy(buildings_shader);
}

// --------------------------------------------------------------------------------

void update_mvp() {
    projection = glm::perspective(glm::radians(camera::zoom), window::aspect_ratio, 1.0f, 1000.0f);
    view = camera::get_view_matrix();
    model = glm::translate(glm::mat4(1.0f), -buildings_model.position);
}

void set_mvp_uniform(Shader &shader) {
    shader.bind();
    shader.set_uniform_mat4("uModel", model);
    shader.set_uniform_mat4("uView", view);
    shader.set_uniform_mat4("uProjection", projection);
};

// --------------------------------------------------------------------------------

void update() {
    update_mvp();

    // Buildings shader update
    set_mvp_uniform(buildings_shader);
    buildings_shader.set_uniform_vec3("uViewPosition", camera::position);
    buildings_shader.set_uniform_vec3("uLightPosition", global::light_position);
    buildings_shader.set_uniform_vec3("uLightColor", global::light_color);

    if (mode == RENDER_MODE_GEOJSON) {
        // Flat shader update
        set_mvp_uniform(flat_shader);
    }

    // Picking shader update
    set_mvp_uniform(picking_shader);

    // Indices shader update
    //set_mvp_uniform(indices_shader);
}

// --------------------------------------------------------------------------------

void clear(glm::vec4 color) {
    GL_CALL(glClearColor(color.r, color.g, color.b, color.a));
    GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

// --------------------------------------------------------------------------------

void render_picking_geojson() {
    global::picking_buffer.bind();

    clear(COLOR_BLACK);

    picking_shader.bind();

    size_t mesh_count = buildings_model.meshes.size() + flat_model.meshes.size();

    for (size_t i = 0; i < buildings_model.meshes.size(); ++i) {
        picking_shader.set_uniform_1ui("uObjectIndex", i + 2);
        picking_shader.set_uniform_1ui("uObjectCount", mesh_count);

        buildings_model.meshes[i].vertex_array.bind();
        GL_CALL(glDrawElements(GL_TRIANGLES, buildings_model.meshes[i].indices.size(), GL_UNSIGNED_INT, nullptr));
        buildings_model.meshes[i].vertex_array.unbind();
    }

    // TODO(paalf): remove this
    for (size_t i = 0; i < flat_model.meshes.size(); ++i) {
        picking_shader.set_uniform_1ui("uObjectIndex", 1);
        picking_shader.set_uniform_1ui("uObjectCount", mesh_count);

        flat_model.meshes[i].vertex_array.bind();
        GL_CALL(glDrawElements(GL_TRIANGLES, flat_model.meshes[i].indices.size(), GL_UNSIGNED_INT, nullptr));
        flat_model.meshes[i].vertex_array.unbind();
    }

    global::picking_buffer.unbind();
}

void render_geojson() {
    buildings_shader.bind();

    clear(COLOR_SLATE);

    size_t mesh_count = buildings_model.meshes.size() + flat_model.meshes.size();

    for (size_t i = 0; i < buildings_model.meshes.size(); ++i) {
        const Mesh &mesh = buildings_model.meshes[i];

        float k = static_cast<float>(i + 2) / mesh_count;
        int cur_id = static_cast<int>(round(255 * CLAMP(k, 0.0f, 1.0f)));

        glm::vec4 mesh_color;
        if (global::picked_id == cur_id) {
            mesh_color = COLOR_RED;

            global::picked_mesh_idx = static_cast<int>(i);
        } else {
            mesh_color = mesh.color;
        }

        if (mesh.type == MESH_TYPE_FLAT) {
            flat_shader.bind();
            flat_shader.set_uniform_vec4("uColor", mesh_color);
        } else {
            buildings_shader.bind();
            buildings_shader.set_uniform_vec4("uColor", mesh_color);
        }

        mesh.vertex_array.bind();
        GL_CALL(glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, nullptr));
        mesh.vertex_array.unbind();
    }

    for (size_t i = 0; i < flat_model.meshes.size(); ++i) {
        const Mesh &mesh = flat_model.meshes[i];

        float k = 1.0f / mesh_count;
        int cur_id = static_cast<int>(round(255 * CLAMP(k, 0.0f, 1.0f)));

        glm::vec4 mesh_color;
        if (global::picked_id == cur_id) {
            mesh_color = COLOR_RED;

            global::picked_mesh_idx = static_cast<int>(i);
        } else {
            mesh_color = mesh.color;
        }

        flat_shader.bind();
        flat_shader.set_uniform_vec4("uColor", mesh_color);

        mesh.vertex_array.bind();
        GL_CALL(glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, nullptr));
        mesh.vertex_array.unbind();
    }
}

void render_picking_collada() {
    global::picking_buffer.bind();

    clear(COLOR_BLACK);

    picking_shader.bind();

    size_t idx_count = building_indices.size();
    uint idx;

    glm::vec3 mesh_color;
    int actual_r, actual_g, actual_b;

    for (size_t i = 0; i < idx_count; ++i) {
        idx = building_indices[i];
        const auto &building_mesh = buildings_model.meshes[idx];

        actual_r = (i + 1) % 256;
        actual_g = ((i + 1) / 256) % 256;
        actual_b = ((i + 1) / (256 * 256)) % 256;

        mesh_color.r = actual_r / 255.0f;
        mesh_color.g = actual_g / 255.0f;
        mesh_color.b = actual_b / 255.0f;

        if (i + 1 == global::picked_id) {
            global::picked_mesh_idx = idx;
        }

        picking_shader.set_uniform_vec3("uObjectColor", mesh_color);

        building_mesh.vertex_array.bind();
        GL_CALL(glDrawElements(GL_TRIANGLES, building_mesh.indices.size(), GL_UNSIGNED_INT, nullptr));
        building_mesh.vertex_array.unbind();
    }

    global::picking_buffer.unbind();
}

void render_indices_collada() {
    global::indices_buffer.bind();

    clear(COLOR_BLACK);

    indices_shader.bind();

    for (auto idx : building_indices) {
        const auto &building_mesh = buildings_model.meshes[idx];

        if (building_mesh.type == MESH_TYPE_BUILDING) {
            indices_shader.set_uniform_vec3("uObjectColor", COLOR_RED);
        }

        if (building_mesh.type == MESH_TYPE_AMENITY) {
            indices_shader.set_uniform_vec3("uObjectColor", COLOR_YELLOW);
        }

        if (building_mesh.type == MESH_TYPE_LANDMARK) {
            indices_shader.set_uniform_vec3("uObjectColor", COLOR_FUCHSIA);
        }

        building_mesh.vertex_array.bind();
        GL_CALL(glDrawElements(GL_TRIANGLES, building_mesh.indices.size(), GL_UNSIGNED_INT, nullptr));
        building_mesh.vertex_array.unbind();
    }

    for (auto idx : tree_indices) {
        const auto &tree_mesh = buildings_model.meshes[idx];

        indices_shader.set_uniform_vec3("uObjectColor", COLOR_GREEN);

        tree_mesh.vertex_array.bind();
        GL_CALL(glDrawElements(GL_TRIANGLES, tree_mesh.indices.size(), GL_UNSIGNED_INT, nullptr));
        tree_mesh.vertex_array.unbind();
    }

    for (auto idx : water_indices) {
        const auto &water_mesh = buildings_model.meshes[idx];

        indices_shader.set_uniform_vec3("uObjectColor", COLOR_BLUE);

        water_mesh.vertex_array.bind();
        GL_CALL(glDrawElements(GL_TRIANGLES, water_mesh.indices.size(), GL_UNSIGNED_INT, nullptr));
        water_mesh.vertex_array.unbind();
    }

    global::indices_buffer.unbind();
}

/*
void render_position_collada() {
    global::position_buffer.bind();

    clear(color::black);

    position_shader.bind();

    size_t mesh_count = models[0].meshes.size();

    for (size_t i = 0; i < mesh_count; ++i) {
        models[0].meshes[i].vertex_array.bind();
        GL_CALL(glDrawElements(GL_TRIANGLES, models[0].meshes[i].indices.size(), GL_UNSIGNED_INT, nullptr));
        models[0].meshes[i].vertex_array.unbind();
    }

    global::position_buffer.unbind();
}
*/

void render_collada() {
    buildings_shader.bind();

    clear(COLOR_SLATE);

    size_t mesh_count = buildings_model.meshes.size();
    size_t idx_count = building_indices.size();

    for (size_t i = 0; i < mesh_count; ++i) {
        const auto &mesh = buildings_model.meshes[i];

        glm::vec4 mesh_color = mesh.color;
        if (global::picked_mesh_idx == i) {
            mesh_color = COLOR_RED;
        }

        buildings_shader.set_uniform_vec4("uColor", mesh_color);

        mesh.vertex_array.bind();
        GL_CALL(glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, nullptr));
        mesh.vertex_array.unbind();
    }
}

// --------------------------------------------------------------------------------

void render() {
    switch (mode) {
    case RENDER_MODE_GEOJSON: render_geojson(); break;
    case RENDER_MODE_COLLADA: render_collada(); break;
    default:                  LOG_ERROR("Unknown render mode.");
    }
}

void render_picking() {
    switch (mode) {
    case RENDER_MODE_GEOJSON: render_picking_geojson(); break;
    case RENDER_MODE_COLLADA: render_picking_collada(); break;
    default:                  LOG_ERROR("Unknown render mode.");
    }
}

void render_indices() {
    switch (mode) {
    //case RENDER_MODE_GEOJSON: render_indices_geojson(); break;
    case RENDER_MODE_COLLADA: render_indices_collada(); break;
    default:                  LOG_ERROR("Unknown render mode.");
    }
}

// --------------------------------------------------------------------------------

void render_menu() {
    menu::setup();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void render_debug_menu() {
    menu::setup_debug();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
} // namespace renderer

// --------------------------------------------------------------------------------

#endif // RENDERER_HPP
