#ifndef OPENGL_HPP
#define OPENGL_HPP

#include "file.hpp"

#include <GL/glew.h>
#include <stb_image/stb_image_write.h>
#include <glm/glm.hpp>

#include <unordered_map>

// --------------------------------------------------------------------------------

#define GLSL_VERSION "#version 330"

// --------------------------------------------------------------------------------

const char *get_gl_error_name(uint err_code) {
    switch (err_code) {
    case GL_NO_ERROR:                                  return "GL_NO_ERROR";
    case GL_INVALID_ENUM:                              return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:                             return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:                         return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:             return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:                             return "GL_OUT_OF_MEMORY";
    case GL_STACK_UNDERFLOW:                           return "GL_STACK_UNDERFLOW";
    case GL_STACK_OVERFLOW:                            return "GL_STACK_OVERFLOW";
    default:                                           return "GL_UNKNOW_ERROR";
    }
}

const char *get_framebuffer_status_name(uint fb_err_code) {
    switch (fb_err_code) {
    case GL_FRAMEBUFFER_COMPLETE:                      return "GL_FRAMEBUFFER_COMPLETE";
    case GL_FRAMEBUFFER_UNDEFINED:                     return "GL_FRAMEBUFFER_UNDEFINED";
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:         return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:        return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:        return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
    case GL_FRAMEBUFFER_UNSUPPORTED:                   return "GL_FRAMEBUFFER_UNSUPPORTED";
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:        return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:      return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
    default:                                           return "GL_FRAMEBUFFER_UNKNOWN_STATUS";
    }
}

void gl_clear_error() {
    while (glGetError() != GL_NO_ERROR);
}

bool gl_log_call(const char *file_path, int line_no, const char *proc_name) {
    while (uint err = glGetError()) {
        fprintf(
            stderr,
            "\x1b[97m%s(%d)\033[0m: OpenGL procedure \x1b[97m'%s'\033[0m: error \x1b[91m%s\033[0m.\n",
            file_path, line_no, proc_name, get_gl_error_name(err)
        );
        return false;
    }
    return true;
}

#ifdef DEBUG_MODE
#   define GL_CALL(proc) \
        gl_clear_error(); \
        proc; \
        if (!gl_log_call(__FILE__, __LINE__, #proc)) { \
            HALT(); \
        }
#else
#   define GL_CALL(proc)
#endif

// --------------------------------------------------------------------------------

void gl_init() {
    ubyte status = glewInit();
    if (status != GLEW_OK) {
        LOG_ERROR("Failed to initialize GLEW: %s", glewGetErrorString(status));
        HALT();
    }

    fprintf(stderr, "-- OpenGL Loaded --\n");
    GL_CALL(fprintf(stderr, "Vendor:   %s\n", glGetString(GL_VENDOR)));
    GL_CALL(fprintf(stderr, "Version:  %s\n", glGetString(GL_VERSION)));
    GL_CALL(fprintf(stderr, "Renderer: %s\n\n", glGetString(GL_RENDERER)));

    GL_CALL(glEnable(GL_DEPTH_TEST));
    GL_CALL(glDisable(GL_BLEND));
}

// --------------------------------------------------------------------------------

struct Vertex_Buffer {
    uint id;

    inline void bind() const   { GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, id)); }
    inline void unbind() const { GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0)); }

    void init(const void *data, size_t size) const {
        bind();
        GL_CALL(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
    }
};

Vertex_Buffer make_vertex_buffer() {
    Vertex_Buffer ret = {};
    GL_CALL(glGenBuffers(1, &ret.id));

    return ret;
}

void destroy(Vertex_Buffer &vb) {
    GL_CALL(glDeleteBuffers(1, &vb.id));
}

// --------------------------------------------------------------------------------

template<typename T>
struct Vertex_Array {
    uint id;
    uint count;

    inline void bind() const   { GL_CALL(glBindVertexArray(id)); }
    inline void unbind() const { GL_CALL(glBindVertexArray(0)); }

    template<typename U>
    void push(uint num_vals, uint offset) {
        static_assert(false, "Unsupported element type.");
    }

    template<>
    void push<float>(uint num_vals, uint offset) {
        GL_CALL(glEnableVertexAttribArray(count));
        GL_CALL(glVertexAttribPointer(count, num_vals, GL_FLOAT, GL_FALSE, sizeof(T),
                                      reinterpret_cast<const void *>(offset)));

        ++count;
    }

    template<>
    void push<uint>(uint num_vals, uint offset) {
        GL_CALL(glEnableVertexAttribArray(count));
        GL_CALL(glVertexAttribPointer(count, num_vals, GL_UNSIGNED_INT, GL_FALSE, sizeof(T),
                                      reinterpret_cast<const void *>(offset)));

        ++count;
    }

    template<>
    void push<ubyte>(uint num_vals, uint offset) {
        GL_CALL(glEnableVertexAttribArray(count));
        GL_CALL(glVertexAttribPointer(count, num_vals, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(T),
                                      reinterpret_cast<const void *>(offset)));

        ++count;
    }
};

template<typename T>
Vertex_Array<T> make_vertex_array() {
    Vertex_Array<T> ret = {};
    GL_CALL(glGenVertexArrays(1, &ret.id));

    return ret;
}

template<typename T>
void destroy(Vertex_Array<T> &va) {
    GL_CALL(glDeleteVertexArrays(1, &va.id));
    va.count = 0;
}

// --------------------------------------------------------------------------------

struct Index_Buffer {
    uint id;

    inline void bind() const   { GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, id)); }
    inline void unbind() const { GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0)); }

    void init(const uint *data, size_t size) {
        GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id));
        GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
    }
};

Index_Buffer make_index_buffer() {
    Index_Buffer ret = {};
    GL_CALL(glGenBuffers(1, &ret.id));

    return ret;
}

void destroy(Index_Buffer &ib) {
    GL_CALL(glDeleteBuffers(1, &ib.id));
}

// --------------------------------------------------------------------------------

struct Frame_Buffer {
    uint id;

    int width;
    int height;

    uint picking_texture;
    uint depth_texture;

    inline void bind() const   { GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, id)); }
    inline void unbind() const { GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0)); }

    void init(int win_width, int win_height, bool store_pos = false) {
        width = win_width;
        height = win_height;

        bind();

        // Create the texture object for the primitive information buffer
        GL_CALL(glGenTextures(1, &picking_texture));
        GL_CALL(glBindTexture(GL_TEXTURE_2D, picking_texture));
        if (store_pos) {
            GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, width, height, 0, GL_RG, GL_FLOAT, nullptr));
        } else {
            GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr));
        }
        GL_CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, picking_texture, 0));

        // Create the texture object for the depth buffer
        GL_CALL(glGenTextures(1, &depth_texture));
        GL_CALL(glBindTexture(GL_TEXTURE_2D, depth_texture));
        GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr));
        GL_CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_texture, 0));

        // Verify that the FBO is correct
        uint status = 0;
        GL_CALL(status = glCheckFramebufferStatus(GL_FRAMEBUFFER));
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            LOG_ERROR("Failed to create frame buffer: %s", get_framebuffer_status_name(status));
            HALT();
        }

        // Restore the default framebuffer
        GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
        unbind();
    }

    void resize(int new_width, int new_height, bool store_pos = false) {
        // Delete existing textures if they exist
        if (picking_texture) {
            GL_CALL(glDeleteTextures(1, &picking_texture));
            picking_texture = 0;
        }

        if (depth_texture) {
            GL_CALL(glDeleteTextures(1, &depth_texture));
            depth_texture = 0;
        }

        // Reinitialize the framebuffer with new dimensions
        init(new_width, new_height, store_pos);
    }

    int read_pixel(int x_pos, int y_pos) const {
        constexpr int num_channels = 4;

        ubyte pixels[num_channels] = {};

        bind();
        GL_CALL(glReadPixels(x_pos, y_pos, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels));
        unbind();

        int picked_id = pixels[0] + pixels[1] * 256 + pixels[2] * (256 * 256);

        return picked_id;
    }

    ubyte *retrieve_color_pixels() const {
        constexpr int num_channels = 4;
        int stride = num_channels * width;
        int count = stride * height;

        ubyte *color_pixels = static_cast<ubyte *>(malloc(count * sizeof(ubyte)));
        ASSERT(color_pixels != nullptr);

        bind();
        GL_CALL(glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, color_pixels));
        unbind();

        return color_pixels;
    }

    float *retrieve_position_pixels() const {
        constexpr int num_channels = 2;
        int stride = num_channels * width;
        int count = stride * height;

        float *pos_pixels = static_cast<float *>(malloc(count * sizeof(float)));
        ASSERT(pos_pixels != nullptr);

        bind();
        GL_CALL(glReadPixels(0, 0, width, height, GL_RG32F, GL_FLOAT, pos_pixels));
        unbind();

        return pos_pixels;
    }

    float *retrieve_depth_pixels() const {
        constexpr int num_channels = 1;
        int stride = num_channels * width;
        int count = stride * height;

        float *depth_pixels = static_cast<float *>(malloc(count * sizeof(float)));
        ASSERT(depth_pixels != nullptr);

        bind();
        GL_CALL(glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth_pixels));
        unbind();

        return depth_pixels;
    }

    void save_image(const char *filepath) const {
        constexpr int num_channels = 4;
        int stride = num_channels * width;
        int count = stride * height;

        ubyte *buff = static_cast<ubyte *>(malloc(count * sizeof(ubyte)));
        ASSERT(buff != nullptr);

        bind();
        GL_CALL(glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buff));
        unbind();

        stbi_write_png(filepath, width, height, num_channels, buff, stride);

        free(buff);
    }
};

Frame_Buffer make_frame_buffer() {
    Frame_Buffer ret = {};
    GL_CALL(glGenFramebuffers(1, &ret.id));

    return ret;
}

void destroy(Frame_Buffer &fb) {
    GL_CALL(glDeleteFramebuffers(1, &fb.id));
}

// --------------------------------------------------------------------------------

struct Shader {
    uint                                 id;
    std::unordered_map<std::string, int> uniform_locations;

    int get_uniform_location(const char *name) {
        auto iter = uniform_locations.find(name);
        if (iter != uniform_locations.end()) {
            return iter->second;
        }

        int location;
        GL_CALL(location = glGetUniformLocation(id, name));
        if (location == -1) {
            LOG_WARNING("Uniform '%s' doesn't exist.", name);
        }

        uniform_locations[name] = location;

        return location;
    }

    inline void bind() const                                      { GL_CALL(glUseProgram(id)); }
    inline void unbind() const                                    { GL_CALL(glUseProgram(0)); }

    inline void set_uniform_1ui(const char *name, uint v0)        { GL_CALL(glUniform1ui(get_uniform_location(name), v0)); }
    inline void set_uniform_vec3(const char *name, glm::vec3 val) { GL_CALL(glUniform3f(get_uniform_location(name), val.x, val.y, val.z)); }
    inline void set_uniform_vec4(const char *name, glm::vec4 val) { GL_CALL(glUniform4f(get_uniform_location(name), val.x, val.y, val.z, val.w)); }
    inline void set_uniform_mat4(const char *name, glm::mat4 val) { GL_CALL(glUniformMatrix4fv(get_uniform_location(name), 1, GL_FALSE, &val[0][0])); }
};

uint compile_shader(uint type, char *content) {
    uint shader_id = 0;
    GL_CALL(shader_id = glCreateShader(type));
    GL_CALL(glShaderSource(shader_id, 1, &content, nullptr));
    GL_CALL(glCompileShader(shader_id));

    int res = GL_FALSE;
    GL_CALL(glGetShaderiv(shader_id, GL_COMPILE_STATUS, &res));
    if (res == GL_FALSE) {
        int size = -1;
        GL_CALL(glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &size));

        char *msg = static_cast<char *>(alloca(size));
        GL_CALL(glGetShaderInfoLog(shader_id, size, &size, msg));

        const char *type_name = nullptr;
        if (type == GL_VERTEX_SHADER) {
            type_name = "vertex";
        } else if (type == GL_FRAGMENT_SHADER) {
            type_name = "fragment";
        } else {
            type_name = "compute";
        }

        LOG_ERROR("Failed to compile %s shader: %s", type_name, msg);

        GL_CALL(glDeleteProgram(shader_id));

        return 0;
    }

    return shader_id;
}

uint link_shader(char *vs_content, char *fs_content) {
    uint program = 0;
    GL_CALL(program = glCreateProgram());
    uint vs_id = compile_shader(GL_VERTEX_SHADER, vs_content);
    uint fs_id = compile_shader(GL_FRAGMENT_SHADER, fs_content);

    GL_CALL(glAttachShader(program, vs_id));
    GL_CALL(glAttachShader(program, fs_id));
    GL_CALL(glLinkProgram(program));
    GL_CALL(glValidateProgram(program));

    GL_CALL(glDeleteShader(vs_id));
    GL_CALL(glDeleteShader(fs_id));

    return program;
}

Shader make_shader(const char *vs_path, const char *fs_path) {
    Shader ret = {};

    char *vs_content = get_file_content(vs_path);
    ASSERT(vs_content != nullptr);

    char *fs_content = get_file_content(fs_path);
    ASSERT(fs_content != nullptr);

    ret.id = link_shader(vs_content, fs_content);

    free(vs_content);
    free(fs_content);

    return ret;
}

void destroy(Shader &s) {
    GL_CALL(glDeleteProgram(s.id));
}

// --------------------------------------------------------------------------------

#endif // OPENGL_HPP
