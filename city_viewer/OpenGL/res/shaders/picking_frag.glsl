#version 330 core
precision highp float;

out vec4 FragColor;

//uniform uint uObjectIndex;
//uniform uint uObjectCount;
uniform vec3 uObjectColor;

void main() {
    //FragColor = vec4(float(uObjectIndex) / float(uObjectCount), 0.0f, 0.0f, 1.0f);
    FragColor = vec4(uObjectColor, 1.0f);
}