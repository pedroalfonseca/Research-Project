#version 330 core

in vec3 position;
out vec3 actualPosition;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main() {
    actualPosition = vec3(uModel * vec4(position, 1.0f));
    gl_Position = uProjection * uView * uModel * vec4(position, 1.0f);
}
