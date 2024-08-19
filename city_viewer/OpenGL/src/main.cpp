#include "platform/indices.hpp"

int main() {
    // Platform initialization
    window::init("City Viewer", 800, 600, "res/icon.png");

    global::init(window::width, window::height);

    indices::init();

    menu::init("Menu", 25.0f, window::handle);

    stbi_set_flip_vertically_on_load(true);
    stbi_flip_vertically_on_write(true);

    renderer::init(RENDER_MODE_COLLADA);

    // Main loop
    while (!window::closed()) {
        if (HAS_FLAG(global::config_flags, CONFIG_FLAGS_ENABLE_CULLING)) {
            GL_CALL(glEnable(GL_CULL_FACE));
            GL_CALL(glCullFace(global::culling_mode));
        } else {
            GL_CALL(glDisable(GL_CULL_FACE));
        }

        window::process_input();

        if (global::show_menu || global::show_debug_menu) {
            menu::new_frame();
        }

        renderer::update();

        // Rendering
        renderer::render_picking();

        // TMP
        //renderer::render_indices();

        renderer::render();

        if (global::show_menu) {
            renderer::render_menu();
        } else if (global::show_debug_menu) {
            renderer::render_debug_menu();
        }

        if (HAS_FLAG(global::config_flags, CONFIG_FLAGS_COMPUTE_INDICES)) {
            indices::compute();
            char a = 0;
        }

        menu::update();

        window::update();
    }

    // Platform shutdown
    renderer::shutdown();

    menu::shutdown();

    global::shutdown();

    window::shutdown();
}

/*
Study:
 - Relief/Bump mapping
 - Compute shaders
 - Static namespace members

Implement a function that computes statistics based on the color buffer:
 - Pixel amount for each object type (including the sky)

Implement a function that computes statistics based on the depth buffer:
 - Min, max, avg depth

Set attributes to the json that we read from:
 - Is the layer a landmark? A natural park? How old is it?

 Create a floating-point texture to store the pixel positions
*/