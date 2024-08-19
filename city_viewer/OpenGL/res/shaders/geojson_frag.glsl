#version 330 core
precision highp float;

in struct {
	vec3 position;
	vec3 normal;
	vec4 color;
} vertex;

in struct {
	vec3 position;
	vec3 color;
} light;

in vec3 viewPosition;

out vec4 fragColor;

void main() {
	// Ambient
	float ambientStrength = 0.1f;
	vec3 ambient = ambientStrength * light.color;

	// Diffuse
	vec3 lightDirection = normalize(light.position - vertex.position);
	float diff = max(dot(vertex.normal, lightDirection), 0.0f);
	vec3 diffuse = diff * light.color;

	// Specular
	float specularStrength = 0.5f;
	vec3 viewDirection = normalize(viewPosition - vertex.position);
	vec3 reflectDirection = reflect(-lightDirection, vertex.normal);
	float spec = pow(max(dot(viewDirection, reflectDirection), 0.0f), 32);
	vec3 specular = specularStrength * spec * light.color;

	fragColor = vec4((ambient + diffuse + specular), 1.0f) * vertex.color;
}