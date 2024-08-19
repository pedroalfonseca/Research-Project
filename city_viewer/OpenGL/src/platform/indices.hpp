#ifndef INDICES_HPP
#define INDICES_HPP

#include "../global.hpp"

#include "renderer.hpp"

// --------------------------------------------------------------------------------

struct View_Indices {
    //   0      1         2        3       4      5
    // [Sky, Building, Amenity, Landmark, Tree, Water]
    uint  color[6];
    float depth;
};

// --------------------------------------------------------------------------------

namespace indices {
const Mesh                                                        *picked_mesh      = nullptr;

std::unordered_map<Camera_Setup, View_Indices, Camera_Setup_Hash>  computed_indices;

std::vector<Camera_Setup>                                          camera_setups;

//Shader                                                             color_shader;
//Shader                                                             depth_shader;

timespec                                                           time_begin;
timespec                                                           time_end;
size_t                                                             memory_usage     = 0;

// --------------------------------------------------------------------------------

size_t set_memory_usage() {
    size_t total_usage = sizeof(computed_indices);

    for (const auto &p : computed_indices) {
        total_usage += sizeof(p.first) + sizeof(p.second);
    }

    size_t ret = total_usage - memory_usage;

    memory_usage = total_usage;

    return ret;
}

// --------------------------------------------------------------------------------

void init() {
    //color_shader = make_shader("res/shaders/color_comp.glsl", global::transient_storage);
    //depth_shader = make_shader("res/shaders/depth_comp.glsl", global::transient_storage);
}

void shutdown() {
    //destroy(color_shader);
    //destroy(depth_shader);
}

// --------------------------------------------------------------------------------

void set_camera_setups() {
    picked_mesh = &renderer::buildings_model.meshes[global::picked_mesh_idx];

    // TODO(paalf): double check this
    camera_setups.reserve(global::granularity[0] * global::granularity[1] * global::granularity[2] * picked_mesh->base_vert_count);

    std::vector<Vertex> verts;
    if (renderer::mode == RENDER_MODE_GEOJSON) {
        verts = picked_mesh->subdivide(global::granularity[0], global::granularity[1]);
    } else {
        verts = picked_mesh->subdivide_aabb(global::granularity);
    }

    uint ares = global::granularity[2];

    float base_yaw;
    float min_yaw, max_yaw;
    float yaw_step = 180.0f / ares;
    float yaw;

    for (const auto &vert : verts) {
        base_yaw = glm::degrees(atan2f(vert.normal.z, vert.normal.x));
        min_yaw = base_yaw - 90.0f, max_yaw = base_yaw + 90.0f;

        for (uint i = 0; i < ares; ++i) {
            yaw = min_yaw + i * yaw_step;
            camera_setups.push_back({vert.position, yaw});
        }
    }
}

// --------------------------------------------------------------------------------

uint get_color_id(const ubyte *color_pixels) {
    if (color_pixels[0] == 255 && color_pixels[1] == 0 && color_pixels[2] == 0) {
        return 1; // Building
    }

    if (color_pixels[0] == 255 && color_pixels[1] == 255 && color_pixels[2] == 0) {
        return 2; // Amenity
    }

    if (color_pixels[0] == 255 && color_pixels[1] == 0 && color_pixels[2] == 255) {
        return 3; // Landmark
    }

    if (color_pixels[0] == 0 && color_pixels[1] == 255 && color_pixels[2] == 0) {
        return 4; // Tree
    }

    if (color_pixels[0] == 0 && color_pixels[1] == 0 && color_pixels[2] == 255) {
        return 5; // Water
    }

    return 0; // Sky
}

// --------------------------------------------------------------------------------

void compute() {
    if (global::saved_experiments.find(global::experiment_name) == global::saved_experiments.end()) {
        global::saved_experiments.emplace(global::experiment_name, global::experiment_name.c_str());
    }

    Experiment &experiment = global::saved_experiments[global::experiment_name];

    timespec_get(&time_begin, TIME_UTC);

    set_camera_setups();

    Camera_Setup original_setup = {camera::position, camera::yaw};

    constexpr uint color_num_channels = 4;
    constexpr uint depth_num_channels = 1;

    int num_pixels = window::width * window::height;

    uint color_len = color_num_channels * num_pixels;
    uint depth_len = depth_num_channels * num_pixels;

    ubyte *mesh_color_pixels = nullptr;
    float *cur_depth_pixels = nullptr;

    char tmp_name[2] = "a";

    for (size_t i = 0; i < camera_setups.size(); ++i) {
        const auto &cur_setup = camera_setups[i];

        camera::position = cur_setup.position;
        camera::set_yaw(cur_setup.yaw);

        // MVP update
        renderer::update_mvp();

        // Indices shader update
        renderer::set_mvp_uniform(renderer::indices_shader);

        renderer::render_indices();

        // TMP
        //save_screenshot(tmp_name, SCREENSHOT_INDICES);
        //++tmp_name[0];

        // Color index computation
        mesh_color_pixels = global::indices_buffer.retrieve_color_pixels();
        uint cur_color_id;
        for (uint j = 0; j < color_len; j += color_num_channels) {
            cur_color_id = get_color_id(mesh_color_pixels + j);
            ++computed_indices[cur_setup].color[cur_color_id];
        }
        free(mesh_color_pixels);

        float sky_rate = static_cast<float>(computed_indices[cur_setup].color[0]) / num_pixels;
        float building_rate = static_cast<float>(computed_indices[cur_setup].color[1]) / num_pixels;
        float amenity_rate = static_cast<float>(computed_indices[cur_setup].color[2]) / num_pixels;
        float landmark_rate = static_cast<float>(computed_indices[cur_setup].color[3]) / num_pixels;
        float tree_rate = static_cast<float>(computed_indices[cur_setup].color[4]) / num_pixels;
        float water_rate = static_cast<float>(computed_indices[cur_setup].color[5]) / num_pixels;

        // Depth index computation
        float min_depth = FLT_MAX;
        float max_depth = FLT_MIN;

        cur_depth_pixels = global::indices_buffer.retrieve_depth_pixels();
        float cur_depth_sum = 0.0f;
        for (uint j = 0; j < depth_len; j += depth_num_channels) {
            min_depth = MIN(min_depth, cur_depth_pixels[j]);
            max_depth = MAX(max_depth, cur_depth_pixels[j]);

            cur_depth_sum += cur_depth_pixels[j];
        }
        free(cur_depth_pixels);

        float avg_depth = cur_depth_sum / (window::width * window::height);

        constexpr float near_ = 1.0f;
        constexpr float far_ = 1000.0f;

        float actual_min_depth = near_ * far_ / (far_ - min_depth * (far_ - near_));
        float actual_max_depth = near_ * far_ / (far_ - max_depth * (far_ - near_));
        float actual_avg_depth = near_ * far_ / (far_ - avg_depth * (far_ - near_));

        computed_indices[cur_setup].depth = actual_avg_depth;

        Camera_Setup actual_cur_setup = cur_setup;
        actual_cur_setup.position -= picked_mesh->aabb.min;

        experiment.write_data(global::picked_id, picked_mesh->aabb.min,
                              actual_cur_setup,
                              building_rate, landmark_rate, amenity_rate, tree_rate, water_rate, sky_rate,
                              actual_min_depth, actual_max_depth, actual_avg_depth);
    }

    camera::position = original_setup.position;
    camera::set_yaw(original_setup.yaw);

    timespec_get(&time_end, TIME_UTC);
    LOG_TRACE("Done computing indices for experiment '%s'.", global::experiment_name);
    double exe_time = (time_end.tv_sec - time_begin.tv_sec) + (time_end.tv_nsec - time_begin.tv_nsec) * 1e-9;

    uint num_cam_setups =
        global::granularity[0] * global::granularity[1] * global::granularity[2] * global::granularity[3]
        * picked_mesh->base_vert_count;

    size_t cur_usage = set_memory_usage();

    experiment.write_performance(global::picked_id, num_cam_setups, exe_time, cur_usage);

    UNSET_FLAG(global::config_flags, CONFIG_FLAGS_COMPUTE_INDICES);

    camera_setups.clear();
}
} // namespace indices

// --------------------------------------------------------------------------------

#endif // INDICES_HPP