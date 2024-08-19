#version 330 core

in vec3 position;
in vec3 normal;

out struct {
	vec3 position;
	vec3 normal;
	vec4 color;
} vertex;

out struct {
	vec3 position;
	vec3 color;
} light;

out vec3 viewPosition;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform vec4 uColor;

uniform vec3 uViewPosition;

uniform vec3 uLightPosition;
uniform vec3 uLightColor;

void main() {
	vertex.position = vec3(uModel * vec4(position, 1.0f));
	vertex.normal = mat3(transpose(inverse(uModel))) * normal;
	vertex.color = uColor;

	light.position = vec3(uModel * vec4(uLightPosition, 1.0f));
	light.color = uLightColor;

	viewPosition = uViewPosition;

	gl_Position = uProjection * uView * vec4(vertex.position, 1.0f);
}