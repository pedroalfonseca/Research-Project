#version 330 core
precision highp float;

out vec4 FragColor;

uniform vec3 uObjectColor;

void main() {
    FragColor = vec4(uObjectColor, 1.0f);
}