#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include "util/geometry.hpp"

// --------------------------------------------------------------------------------

enum _Config_Flags : uint {
    // Camera
    CONFIG_FLAGS_CONTROL_CAMERA         = BIT(1),
    CONFIG_FLAGS_ATTACH_LIGHT_TO_CAMERA = BIT(2),

    // Indices
    CONFIG_FLAGS_COMPUTE_INDICES        = BIT(3),

    // Render
    CONFIG_FLAGS_ENABLE_CULLING         = BIT(4),
    CONFIG_FLAGS_ENABLE_WIREFRAME       = BIT(5),

    // Menu
    CONFIG_FLAGS_SHOW_POPUP             = BIT(6),
};

typedef uint Config_Flags;

// Presets
#define CONFIG_FLAGS_ALL_UNSET 0u
#define CONFIG_FLAGS_DEFAULT   (CONFIG_FLAGS_CONTROL_CAMERA | \
                                CONFIG_FLAGS_ATTACH_LIGHT_TO_CAMERA | \
                                CONFIG_FLAGS_ENABLE_CULLING)

// --------------------------------------------------------------------------------

enum Screenshot_Mode : ubyte {
    SCREENSHOT_MODE_PICKING,
    SCREENSHOT_MODE_INDICES,
    SCREENSHOT_MODE_AABB
};

// --------------------------------------------------------------------------------

struct Camera_Setup {
    glm::vec3 position;
    float     yaw;
};

inline bool operator==(Camera_Setup a, Camera_Setup b) {
    return a.position == b.position && a.yaw == b.yaw;
}

struct Camera_Setup_Hash {
    size_t operator()(const Camera_Setup &setup) const {
        size_t h1 = std::hash<float>()(setup.yaw);
        size_t h2 = std::hash<float>()(setup.position.x);
        size_t h3 = std::hash<float>()(setup.position.y);
        size_t h4 = std::hash<float>()(setup.position.z);
        
        // Combine all the hash values
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
    }
};

// --------------------------------------------------------------------------------

struct Experiment {
    FILE *performance_file;
    FILE *data_file;

    Experiment() : performance_file(nullptr), data_file(nullptr) {}

    Experiment(const char *exp_name) {
        ASSERT(exp_name != nullptr);
        size_t name_len = strlen(exp_name);

        size_t dir_path_len = 18 + name_len + 1;
        char *dir_path = static_cast<char *>(malloc(dir_path_len));
        ASSERT(dir_path);
        sprintf(dir_path, "files/experiments/%s", exp_name);

        if (!create_directory(dir_path)) {
            return;
        }

        char *performance_path = static_cast<char *>(malloc(dir_path_len + 1 + name_len + 16 + 1));
        ASSERT(performance_path);
        sprintf(performance_path, "%s/%s_performance.csv", dir_path, exp_name);

        performance_file = fopen(performance_path, "wb");
        ASSERT(performance_file != nullptr);

        free(performance_path);

        fprintf(performance_file, "building_id,num_camera_setups,execution_time,memory_usage\n");

        char *data_path = static_cast<char *>(malloc(dir_path_len + 1 + name_len + 9 + 1));
        ASSERT(data_path);
        sprintf(data_path, "%s/%s_data.csv", dir_path, exp_name);

        data_file = fopen(data_path, "wb");
        ASSERT(data_file != nullptr);

        free(data_path);

        fprintf(
            data_file,
            "building_id,origin_x,origin_y,origin_z,"
            "x,y,z,yaw,"
            "building_rate,landmark_rate,amenity_rate,tree_rate,water_rate,sky_rate,"
            "min_depth,max_depth,avg_depth\n"
        );
    }

    ~Experiment() {
        if (performance_file != nullptr) {
            fclose(performance_file);
        }

        if (data_file != nullptr) {
            fclose(data_file);
        }
    }

    void write_performance(int picked_id, uint num_cam_setups, double exe_time, size_t mem_usage) {
        ASSERT(performance_file);

        fprintf(performance_file, "%d,%u,%lf,%zu\n", picked_id, num_cam_setups, exe_time, mem_usage);
        fflush(performance_file);
    }

    void write_data(int picked_id, glm::vec3 origin_pos,
                    Camera_Setup cam_setup,
                    float building_rate, float landmark_rate, float amenity_rate, float tree_rate, float water_rate, float sky_rate,
                    float min_depth, float max_depth, float avg_depth) {
        ASSERT(data_file);

        fprintf(
            data_file,
            "%d,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n",
            picked_id, origin_pos.x, origin_pos.y, origin_pos.z,
            cam_setup.position.x, cam_setup.position.y, cam_setup.position.z, cam_setup.yaw,
            building_rate, landmark_rate, amenity_rate, tree_rate, water_rate, sky_rate,
            min_depth, max_depth, avg_depth
        );
    }
};

// --------------------------------------------------------------------------------

namespace global {
// Platform
Config_Flags    config_flags         = CONFIG_FLAGS_ALL_UNSET;
FILE           *log_file             = nullptr;

// Light
glm::vec3       light_position       = {};
glm::vec3       light_color          = COLOR_WHITE;

// Model
glm::vec3       model_origin         = {};

// Indices
Frame_Buffer    picking_buffer;
Frame_Buffer    indices_buffer;
Frame_Buffer    position_buffer;
Frame_Buffer    aabb_buffer;

int             picked_id            = -1;
int             picked_mesh_idx      = -1;

int             granularity[4]       = {2, 2, 2, 3};

std::unordered_map<std::string, Experiment> saved_experiments;
std::string experiment_name;

// Debug
uint            culling_mode         = GL_FRONT;
const char     *culling_mode_name    = "Front";

Screenshot_Mode screenshot_mode      = SCREENSHOT_MODE_PICKING;
const char     *screenshot_mode_name = "Picking";

// Menu
bool            show_menu            = false;
bool            show_debug_menu      = false;

// --------------------------------------------------------------------------------

void init(int win_width, int win_height) {
    config_flags = CONFIG_FLAGS_DEFAULT;

#ifndef DEBUG_MODE
    log_file = fopen("files/log.txt", "wb");
    ASSERT(log_file != nullptr);
#endif // DEBUG_MODE

    picking_buffer = make_frame_buffer();
    picking_buffer.init(win_width, win_height);

    indices_buffer = make_frame_buffer();
    indices_buffer.init(win_width, win_height);

    position_buffer = make_frame_buffer();
    position_buffer.init(win_width, win_height, true);
}

void shutdown() {
    destroy(position_buffer);
    destroy(indices_buffer);
    destroy(picking_buffer);

#ifndef DEBUG_MODE
    fclose(log_file);
#endif // DEBUG_MODE
}
} // namespace global

// --------------------------------------------------------------------------------

bool save_screenshot(const char *screenshot_name, Screenshot_Mode screenshot_mode = global::screenshot_mode) {
    if (screenshot_name == nullptr) {
        return false;
    }

    size_t screenshot_name_len = strlen(screenshot_name);

    char *base_dir_path = get_current_great_grandparent_directory();
    if (base_dir_path == nullptr) {
        return false;
    }

    size_t base_dir_path_len = strlen(base_dir_path);

    size_t screenshot_dir_path_len = base_dir_path_len + 1 + 11 + 1;
    char *screenshot_dir_path = static_cast<char *>(alloca(screenshot_dir_path_len));

#ifdef _WIN32
    snprintf(screenshot_dir_path, screenshot_dir_path_len, "%s\\screenshots", base_dir_path);
#else
    snprintf(screenshot_dir_path, screenshot_dir_path_len, "%s/screenshots", base_dir_path);
#endif // _WIN32

    size_t screenshot_path_len = screenshot_dir_path_len + 1 + screenshot_name_len + 4 + 1;
    char *screenshot_path = static_cast<char *>(alloca(screenshot_path_len));

#ifdef _WIN32
    snprintf(screenshot_path, screenshot_path_len, "%s\\%s.png", screenshot_dir_path, screenshot_name);
#else
    snprintf(screenshot_path, screenshot_path_len, "%s/%s.png", screenshot_dir_path, screenshot_name);
#endif // _WIN32

    if (screenshot_mode == SCREENSHOT_MODE_PICKING) {
        global::picking_buffer.save_image(screenshot_path);
    } else if (screenshot_mode == SCREENSHOT_MODE_INDICES) {
        global::indices_buffer.save_image(screenshot_path);
    } else {
        //global::aabb_buffer.save_image(screenshot_path);
    }

    LOG_TRACE("Successfully saved frame buffer screenshot '%s'.", screenshot_name);

    return true;
}

// --------------------------------------------------------------------------------

#endif // GLOBAL_HPP