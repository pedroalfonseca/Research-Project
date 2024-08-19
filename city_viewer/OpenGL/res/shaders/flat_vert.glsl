#version 330 core

in vec3 position;

out vec4 color;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform vec4 uColor;

void main() {
	color = uColor;
    gl_Position = uProjection * uView * uModel * vec4(position, 1.0f);
}