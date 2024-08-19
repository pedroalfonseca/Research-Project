#version 330 core
precision highp float;

in vec4 color;

out vec4 fragColor;

void main() {
	fragColor = color;
}