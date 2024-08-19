#ifndef MENU_HPP
#define MENU_HPP

#include "camera.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

// --------------------------------------------------------------------------------

namespace menu {
ImGuiIO            *io         = nullptr;

const char         *title      = nullptr;
float               padding    = 0.0f;

float               delta_time = 0.0f;
float               last_frame = 0.0f;

std::vector<float>  frame_times;

// --------------------------------------------------------------------------------

void init(const char *menu_title, float menu_padding, GLFWwindow *window) {
    title = menu_title;
    padding = menu_padding;

    frame_times.reserve(100);

    ImGui::CreateContext();
    io = &ImGui::GetIO();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(GLSL_VERSION);
    ImGui::StyleColorsDark();
}

void shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

// --------------------------------------------------------------------------------

void new_frame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

// --------------------------------------------------------------------------------

void setup() {
    // Menu window
    ImGui::SetNextWindowPos(ImVec2(padding, padding));
    ImGui::SetNextWindowSize(ImVec2(io->DisplaySize.x - 2.0f * padding, io->DisplaySize.y - 2.0f * padding));

    ImGui::Begin(title, &global::show_menu, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    // Camera section
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();

        if (ImGui::Button("Move To Model Origin")) {
            camera::position = global::model_origin;
        }

        ImGui::Spacing();

        ImGui::InputFloat3("Camera Position", &camera::position[0]);

        ImGui::Spacing();
    }

    // Light section
    if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();

        if (ImGui::CheckboxFlags("Attach to Camera", &global::config_flags, CONFIG_FLAGS_ATTACH_LIGHT_TO_CAMERA)) {
            *camera::light_position_ptr = camera::position;
        }

        ImGui::Spacing();

        if (ImGui::Button("Move To Camera Position")) {
            global::light_position = camera::position;
        }

        ImGui::Spacing();

        ImGui::InputFloat3("Light Position", &global::light_position[0]);

        ImGui::Spacing();

        ImGui::ColorEdit3("Light Color", &global::light_color[0]);

        ImGui::Spacing();
    }

    // Indices section
    if (ImGui::CollapsingHeader("Indices", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();

        if (ImGui::Button("Create Experiment")) {
            ImGui::OpenPopup("Enter Experiment Name");
            SET_FLAG(global::config_flags, CONFIG_FLAGS_SHOW_POPUP);
        }

        ImGui::Spacing();

        if (ImGui::BeginPopupModal("Enter Experiment Name", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
            ImVec2 popup_size = ImGui::GetWindowSize();
            ImVec2 popup_pos = ImVec2((io->DisplaySize.x - popup_size.x) / 2.0f, (io->DisplaySize.y - popup_size.y) / 2.0f);
            ImGui::SetWindowPos(popup_pos);

            constexpr size_t max_exp_name_len = 25;
            static char exp_name[max_exp_name_len] = {};
            ImGui::InputText("##exp_name", exp_name, max_exp_name_len);

            if (ImGui::Button("Ok")) {
                if (strlen(exp_name) == 0) {
                    LOG_ERROR("Experiment name cannot be empty.");
                }

                global::saved_experiments.emplace(exp_name, exp_name);

                if (global::experiment_name.empty()) {
                    global::experiment_name = exp_name;
                }

                ImGui::CloseCurrentPopup();
                UNSET_FLAG(global::config_flags, CONFIG_FLAGS_SHOW_POPUP);
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
                UNSET_FLAG(global::config_flags, CONFIG_FLAGS_SHOW_POPUP);
            }

            ImGui::EndPopup();
        }

        ImGui::Spacing();

        const char* dir_path = "files/experiments"; // Update with your directory path
        std::vector<std::string> subdirectories = get_subdirectories(dir_path);

        if (ImGui::BeginCombo("##selected_experiment_name", global::experiment_name.c_str())) {
            //for (auto &p : global::saved_experiments) {
            //    bool selected = (global::experiment_name == p.first.c_str());
            //    if (ImGui::Selectable(p.first.c_str(), selected)) {
            //        global::experiment_name = p.first.c_str();
            for (const auto &dir : subdirectories) {
                bool selected = (global::experiment_name == dir);
                if (ImGui::Selectable(dir.c_str(), selected)) {
                    global::experiment_name = dir;
                }

                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::Text("Selected Experiment");

        ImGui::Spacing();

        if (ImGui::Button("Compute Indices")) {
            if (global::picked_mesh_idx < 0) {
                LOG_ERROR("No building was selected.");
            } else {
                ImGui::OpenPopup("Enter Granularity");
                SET_FLAG(global::config_flags, CONFIG_FLAGS_SHOW_POPUP);
            }
        }

        if (ImGui::BeginPopupModal("Enter Granularity", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
            ImVec2 popup_size = ImGui::GetWindowSize();
            ImVec2 popup_pos = ImVec2((io->DisplaySize.x - popup_size.x) / 2.0f, (io->DisplaySize.y - popup_size.y) / 2.0f);
            ImGui::SetWindowPos(popup_pos);

            ImGui::InputInt4("##granularity", global::granularity);

            if (ImGui::Button("Compute")) {
                if (global::granularity[0] <= 0 || global::granularity[1] <= 0 ||
                    global::granularity[2] <= 0 || global::granularity[3] <= 0) {
                    LOG_ERROR("Invalid granularity.");
                } else {
                    SET_FLAG(global::config_flags, CONFIG_FLAGS_COMPUTE_INDICES);

                    ImGui::CloseCurrentPopup();
                    UNSET_FLAG(global::config_flags, CONFIG_FLAGS_SHOW_POPUP);
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
                UNSET_FLAG(global::config_flags, CONFIG_FLAGS_SHOW_POPUP);
            }

            ImGui::EndPopup();
        }

        ImGui::Spacing();
    }

    // Debug section
    if (ImGui::CollapsingHeader("Debug", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();

        ImGui::Checkbox("Show Debug Menu", &global::show_debug_menu);

        ImGui::Spacing();
    }

    ImGui::End();
}

void setup_debug() {
    // Debug window
    float width = io->DisplaySize.x / 4.0f;

    ImGui::SetNextWindowPos(ImVec2(io->DisplaySize.x - width - padding, padding), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width, io->DisplaySize.y / 2.0f), ImGuiCond_Always);

    ImGui::Begin("Debug Menu", &global::show_debug_menu, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    // Render section
    if (ImGui::CollapsingHeader("Render", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        static uint culling_modes[] = {GL_FRONT, GL_BACK, GL_FRONT_AND_BACK};
        static const char *culling_mode_names[] = {"Front", "Back", "Front And Back"};

        ImGui::CheckboxFlags("Enable Culling", &global::config_flags, CONFIG_FLAGS_ENABLE_CULLING);

        if (HAS_FLAG(global::config_flags, CONFIG_FLAGS_ENABLE_CULLING)) {
            ImGui::Spacing();

            if (ImGui::BeginCombo("##culling_mode", global::culling_mode_name)) {
                for (size_t i = 0; i < ARRAY_SIZE(culling_modes); ++i) {
                    bool selected = (culling_modes[i] == global::culling_mode);
                    if (ImGui::Selectable(culling_mode_names[i], selected)) {
                        global::culling_mode = culling_modes[i];
                        global::culling_mode_name = culling_mode_names[i];
                    }

                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }
            ImGui::SameLine();
            ImGui::Text("Culling Mode");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::CheckboxFlags("Enable Wireframe", &global::config_flags, CONFIG_FLAGS_ENABLE_WIREFRAME)) {
            GL_CALL(glPolygonMode(GL_FRONT_AND_BACK, HAS_FLAG(global::config_flags, CONFIG_FLAGS_ENABLE_WIREFRAME) ? GL_LINE : GL_FILL));
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        static const char *screenshot_mode_names[] = {"Picking", "Indices", "AABB"};

        ImGui::Spacing();

        if (ImGui::BeginCombo("##screenshot_mode", global::screenshot_mode_name)) {
            for (size_t i = 0; i < ARRAY_SIZE(screenshot_mode_names); ++i) {
                bool selected = (i == global::screenshot_mode);
                if (ImGui::Selectable(screenshot_mode_names[i], selected)) {
                    global::screenshot_mode = static_cast<Screenshot_Mode>(i);
                    global::screenshot_mode_name = screenshot_mode_names[i];
                }

                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::Text("Screenshot Mode");

        if (ImGui::Button("Save Screenshot")) {
            ImGui::OpenPopup("Enter Screenshot Name");
            SET_FLAG(global::config_flags, CONFIG_FLAGS_SHOW_POPUP);
        }

        if (ImGui::BeginPopupModal("Enter Screenshot Name", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
            ImVec2 popup_size = ImGui::GetWindowSize();
            ImVec2 popup_pos = ImVec2((io->DisplaySize.x - popup_size.x) / 2.0f, (io->DisplaySize.y - popup_size.y) / 2.0f);
            ImGui::SetWindowPos(popup_pos);

            constexpr size_t max_screenshot_name_len = 25;
            static char screenshot_name[max_screenshot_name_len] = {};
            ImGui::InputText("##screenshot_name", screenshot_name, max_screenshot_name_len);

            if (ImGui::Button("Ok")) {
                if (strlen(screenshot_name) == 0) {
                    LOG_ERROR("Screenshot name cannot be empty.");
                } else if (save_screenshot(screenshot_name)) {
                    ImGui::CloseCurrentPopup();
                    UNSET_FLAG(global::config_flags, CONFIG_FLAGS_SHOW_POPUP);
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
                UNSET_FLAG(global::config_flags, CONFIG_FLAGS_SHOW_POPUP);
            }

            ImGui::EndPopup();
        }

        ImGui::Spacing();
    }

    // Camera section
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();

        ImGui::BeginTable("Camera Info", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);

        ImGui::TableSetupColumn("Parameter", ImGuiTableColumnFlags_WidthFixed, 60.0f);

        // Position
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Position");
        ImGui::TableNextColumn();
        ImGui::Text("x = %.2f\ny = %.2f\nz = %.2f", camera::position.x, camera::position.y, camera::position.z);

        // Yaw
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Yaw");
        ImGui::TableNextColumn();
        ImGui::Text("%.2f", camera::yaw);

        // Pitch
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Pitch");
        ImGui::TableNextColumn();
        ImGui::Text("%.2f", camera::pitch);

        // Zoom
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Zoom");
        ImGui::TableNextColumn();
        ImGui::Text("%.2f", camera::zoom);

        ImGui::EndTable();

        ImGui::Spacing();
    }

    // Picking section
    if (ImGui::CollapsingHeader("Picking", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();

        ImGui::BeginTable("Picking Info", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);

        ImGui::TableSetupColumn("Parameter", ImGuiTableColumnFlags_WidthFixed, 120.0f);

        // Picked ID
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Picked ID");
        ImGui::TableNextColumn();
        ImGui::Text("%d", global::picked_id);

        // Picked mesh index
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Picked Mesh Index");
        ImGui::TableNextColumn();
        ImGui::Text("%d", global::picked_mesh_idx);

        ImGui::EndTable();

        ImGui::Spacing();
    }

    // Performance section
    if (ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();

        ImGui::BeginTable("Performance Info", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);

        ImGui::TableSetupColumn("Parameter", ImGuiTableColumnFlags_WidthFixed, 50.0f);

        // Application Average
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("App Avg");
        ImGui::TableNextColumn();
        ImGui::Text("%.3f ms/frame", 1000.0f / io->Framerate);

        // FPS
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("FPS");
        ImGui::TableNextColumn();
        ImGui::Text("%.1f", io->Framerate);

        ImGui::EndTable();

        ImGui::Spacing();

        ImGui::PlotLines("FPS Over Time", frame_times.data(), 100, 0, nullptr, 0.0f, ImGui::GetTime());

        ImGui::Spacing();
    }

    ImGui::End();
}

// --------------------------------------------------------------------------------

void update() {
    float cur_frame = static_cast<float>(ImGui::GetTime());
    delta_time = cur_frame - last_frame;
    last_frame = cur_frame;

    frame_times.push_back(1.0f / delta_time);

    if (frame_times.size() > 100) {
        frame_times.erase(frame_times.begin());
    }
}
} // namespace menu

// --------------------------------------------------------------------------------

#endif // MENU_HPP