#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "../global.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// --------------------------------------------------------------------------------

#define YAW         -90.0f
#define PITCH         0.0f
#define SPEED        35.0f
#define SENSITIVITY   0.1f
#define ZOOM         45.0f
#define MAX_ZOOM     90.0f

// --------------------------------------------------------------------------------

enum Camera_Movement : ubyte {
    CAMERA_MOVEMENT_LEFT,
    CAMERA_MOVEMENT_RIGHT,
    CAMERA_MOVEMENT_BACKWARD,
    CAMERA_MOVEMENT_FORWARD
};

// --------------------------------------------------------------------------------

namespace camera {
glm::vec3  position;
glm::vec3 *light_position_ptr;

glm::vec3  right;
glm::vec3  front              = {0.0f, 0.0f, -1.0f};
glm::vec3  up;
glm::vec3  world_up;

float      yaw                = YAW;
float      pitch              = PITCH;

float      movement_speed     = SPEED;
float      mouse_sensitivity  = SENSITIVITY;
float      zoom               = ZOOM;

// --------------------------------------------------------------------------------

void update_basis() {
    float front_x = cosf(glm::radians(yaw)) * cosf(glm::radians(pitch));
    float front_y = sinf(glm::radians(pitch));
    float front_z = sinf(glm::radians(yaw)) * cosf(glm::radians(pitch));
    front = glm::normalize(glm::vec3(front_x, front_y, front_z));

    right = glm::normalize(glm::cross(front, world_up));
    up = glm::normalize(glm::cross(right, front));
}

void init(glm::vec3 pos = {}, glm::vec3 w_up = {0.0f, 1.0f, 0.0f}) {
    position = pos;
    world_up = w_up;
    update_basis();
}

// --------------------------------------------------------------------------------

inline glm::mat4 get_view_matrix()   { return glm::lookAt(position, position + front, up); }

inline void set_zoom(float new_zoom) { zoom = MAX_ZOOM - new_zoom; }

void set_yaw(float new_yaw) {
    yaw = new_yaw;
    update_basis();
}

void set_pitch(float new_pitch) {
    pitch = new_pitch;
    update_basis();
}

// --------------------------------------------------------------------------------

void process_keyboard(Camera_Movement direction, float delta_time) {
    float velocity = movement_speed * delta_time;

    switch (direction) {
    case CAMERA_MOVEMENT_LEFT: {
        position -= right * velocity;

        if (HAS_FLAG(global::config_flags, CONFIG_FLAGS_ATTACH_LIGHT_TO_CAMERA)) {
            *light_position_ptr -= right * velocity;
        }
    } break;

    case CAMERA_MOVEMENT_RIGHT: {
        position += right * velocity;

        if (HAS_FLAG(global::config_flags, CONFIG_FLAGS_ATTACH_LIGHT_TO_CAMERA)) {
            *light_position_ptr += right * velocity;
        }
    } break;

    case CAMERA_MOVEMENT_BACKWARD: {
        position -= front * velocity;

        if (HAS_FLAG(global::config_flags, CONFIG_FLAGS_ATTACH_LIGHT_TO_CAMERA)) {
            *light_position_ptr -= front * velocity;
        }
    } break;

    case CAMERA_MOVEMENT_FORWARD: {
        position += front * velocity;

        if (HAS_FLAG(global::config_flags, CONFIG_FLAGS_ATTACH_LIGHT_TO_CAMERA)) {
            *light_position_ptr += front * velocity;
        }
    } break;
    }
}

void process_mouse_movement(float x_offset, float y_offset,
                            bool constrain_pitch = true) {
    yaw += x_offset * mouse_sensitivity;
    pitch += y_offset * mouse_sensitivity;

    yaw = fmodf(yaw, 360.0f);

    if (constrain_pitch) {
        pitch = CLAMP(pitch, -89.0f, 89.0f);
    }

    update_basis();
}

void process_mouse_scroll(float y_offset) {
    zoom -= y_offset;
    zoom = CLAMP(zoom, 1.0f, MAX_ZOOM);
}
} // namespace camera

// --------------------------------------------------------------------------------

#endif // CAMERA_HPP