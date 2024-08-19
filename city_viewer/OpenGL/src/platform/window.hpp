#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "camera.hpp"
#include "menu.hpp"

#include <GLFW/glfw3.h>
#include <stb_image/stb_image.h>

// --------------------------------------------------------------------------------

namespace window {
const char *get_glfw_error_name(uint err_code) {
    switch (err_code) {
    case GLFW_NO_ERROR:            return "GLFW_NO_ERROR";
    case GLFW_NOT_INITIALIZED:     return "GLFW_NOT_INITIALIZED";
    case GLFW_NO_CURRENT_CONTEXT:  return "GLFW_NO_CURRENT_CONTEXT";
    case GLFW_INVALID_ENUM:        return "GLFW_INVALID_ENUM";
    case GLFW_INVALID_VALUE:       return "GLFW_INVALID_VALUE";
    case GLFW_OUT_OF_MEMORY:       return "GLFW_OUT_OF_MEMORY";
    case GLFW_API_UNAVAILABLE:     return "GLFW_API_UNAVAILABLE";
    case GLFW_VERSION_UNAVAILABLE: return "GLFW_VERSION_UNAVAILABLE";
    case GLFW_PLATFORM_ERROR:      return "GLFW_PLATFORM_ERROR";
    case GLFW_FORMAT_UNAVAILABLE:  return "GLFW_FORMAT_UNAVAILABLE";
    case GLFW_NO_WINDOW_CONTEXT:   return "GLFW_NO_WINDOW_CONTEXT";
    default:                       return "GLFW_UNKNOW_ERROR";
    }
}

// --------------------------------------------------------------------------------

GLFWwindow *handle        = nullptr;
int         width;
int         height;
float       aspect_ratio;

bool        esc_processed = false;

float       center_x;
float       center_y;

float       delta_time    = 0.0f;
float       last_frame    = 0.0f;

// --------------------------------------------------------------------------------

void framebuffer_size_callback(GLFWwindow *window, int win_width, int win_height) {
    width = win_width;
    height = win_height;
    aspect_ratio = static_cast<float>(width) / height;

    center_x = width / 2.0f;
    center_y = height / 2.0f;

    glfwSetCursorPos(window, center_x, center_y);

    global::picking_buffer.resize(win_width, win_height);
    global::indices_buffer.resize(win_width, win_height);
    global::position_buffer.resize(win_width, win_height, true);

    glViewport(0, 0, width, height);

    menu::io->DisplaySize.x = static_cast<float>(width);
    menu::io->DisplaySize.y = static_cast<float>(height);
}

void mouse_callback(GLFWwindow *window, double x_pos_in, double y_pos_in) {
    if (HAS_FLAG(global::config_flags, CONFIG_FLAGS_CONTROL_CAMERA) && !global::show_menu) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        float x_pos = static_cast<float>(x_pos_in);
        float y_pos = static_cast<float>(y_pos_in);

        float x_offset = x_pos - center_x;
        float y_offset = center_y - y_pos;

        camera::process_mouse_movement(x_offset, y_offset);

        glfwSetCursorPos(window, center_x, center_y);
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    if (!global::show_menu) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            double x_pos_in;
            double y_pos_in;

            glfwGetCursorPos(window, &x_pos_in, &y_pos_in);

            int x_pos = static_cast<int>(x_pos_in);
            int y_pos = static_cast<int>(y_pos_in);

            if (!HAS_FLAG(global::config_flags, CONFIG_FLAGS_CONTROL_CAMERA) &&
                !HAS_FLAG(global::config_flags, CONFIG_FLAGS_SHOW_POPUP)) {
                int tmp = global::picking_buffer.read_pixel(x_pos, height - y_pos - 1);
                if (global::picked_id == tmp || tmp == 0) {
                    global::picked_id = -1;
                    global::picked_mesh_idx = -1;
                } else {
                    global::picked_id = tmp;
                }
            }
        }

        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
            FLIP_FLAG(global::config_flags, CONFIG_FLAGS_CONTROL_CAMERA);
            LOG_TRACE("Control camera: %s", HAS_FLAG(global::config_flags, CONFIG_FLAGS_CONTROL_CAMERA) ? "ON" : "OFF");
        }
    }
}

void scroll_callback(GLFWwindow *window, double x_offset, double y_offset) {
    if (!global::show_menu && HAS_FLAG(global::config_flags, CONFIG_FLAGS_CONTROL_CAMERA)) {
        camera::process_mouse_scroll(static_cast<float>(y_offset));
    }
}

// --------------------------------------------------------------------------------

void process_input() {
    if (glfwGetKey(handle, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        if (!esc_processed) {
            global::show_menu = !global::show_menu;
            esc_processed = true;
        }
    } else {
        esc_processed = false;
    }

    if (!global::show_menu && HAS_FLAG(global::config_flags, CONFIG_FLAGS_CONTROL_CAMERA)) {
        float current_frame = static_cast<float>(glfwGetTime());
        delta_time = current_frame - last_frame;
        last_frame = current_frame;

        if (glfwGetKey(handle, GLFW_KEY_A) == GLFW_PRESS) {
            camera::process_keyboard(CAMERA_MOVEMENT_LEFT, delta_time);
        }

        if (glfwGetKey(handle, GLFW_KEY_D) == GLFW_PRESS) {
            camera::process_keyboard(CAMERA_MOVEMENT_RIGHT, delta_time);
        }

        if (glfwGetKey(handle, GLFW_KEY_S) == GLFW_PRESS) {
            camera::process_keyboard(CAMERA_MOVEMENT_BACKWARD, delta_time);
        }

        if (glfwGetKey(handle, GLFW_KEY_W) == GLFW_PRESS) {
            camera::process_keyboard(CAMERA_MOVEMENT_FORWARD, delta_time);
        }
    }
}

// --------------------------------------------------------------------------------

void shutdown() {
    if (handle != nullptr) {
        glfwDestroyWindow(handle);
    }
    glfwTerminate();
}

void init(const char *title, int win_width, int win_height, const char *icon_path, bool enable_vsync = true) {
    if (glfwInit() == GLFW_FALSE) {
        LOG_ERROR("Failed to initialize GLFW.");
        shutdown();
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
#endif

    handle = glfwCreateWindow(win_width, win_height, title, nullptr, nullptr);
    if (handle == nullptr) {
        const char *description = nullptr;
        LOG_ERROR("Failed to create GLFW window: %s.", get_glfw_error_name(glfwGetError(&description)));
        shutdown();
    }

    width = win_width;
    height = win_height;
    aspect_ratio = static_cast<float>(width) / height;

    center_x = width / 2.0f;
    center_y = height / 2.0f;

    glfwSetCursorPos(handle, center_x, center_y);

    glfwMakeContextCurrent(handle);

    glfwSetFramebufferSizeCallback(handle, framebuffer_size_callback);
    glfwSetCursorPosCallback(handle, mouse_callback);
    glfwSetMouseButtonCallback(handle, mouse_button_callback);
    glfwSetScrollCallback(handle, scroll_callback);

    glfwSetInputMode(handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSwapInterval(enable_vsync);

    GLFWimage icons[1];
    icons[0].pixels = stbi_load(icon_path, &icons[0].width, &icons[0].height, 0, 4);
    if (icons[0].pixels == nullptr) {
        LOG_ERROR("Failed to load window icon '%s'.", icon_path);
        shutdown();
    }

    glfwSetWindowIcon(handle, 1, icons);

    free(icons[0].pixels);

    gl_init();
}

void update() {
    glfwSwapBuffers(handle);
    glfwPollEvents();
}

bool closed() {
    return glfwWindowShouldClose(handle);
}
} // namespace window

// --------------------------------------------------------------------------------

#endif // WINDOW_HPP
