#version 330 core

in vec3 actualPosition;
out vec2 fragColor;

uniform uint uObjectIndex;
uniform uint uObjectCount;

void main() {
    fragColor = vec2(actualPosition.x, actualPosition.y);
}