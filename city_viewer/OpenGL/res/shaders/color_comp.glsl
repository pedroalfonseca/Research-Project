#version 430

layout(std430, binding = 0) buffer ColorBuffer {
	float colorPixels[];
};

layout(std430, binding = 1) buffer IndicesBuffer {
	uint colorIndices[];
};

uniform uint color_len;
uniform uint color_num_channels;

uint get_mesh_type(float r, float g, float b) {
	if (r == 1.0f && g == 0.0f && b == 0.0f) {
		return 1; // Building
	}

	if (r == 1.0f && g == 1.0f && b == 0.0f) {
		return 2; // Amenity
	}

	if (r == 1.0f && g == 0.0f && b == 1.0f) {
		return 3; // Landmark
	}

	if (r == 0.0f && g == 1.0f && b == 0.0f) {
		return 4; // Tree
	}

	if (r == 0.0f && g == 0.0f && b == 1.0f) {
		return 5; // Water
	}

	return 0; // Sky
}

void main() {
	uint idx = gl_GlobalInvocationID.x * color_num_channels;
	if (idx >= color_len) {
		return;
	}

	float r = colorPixels[idx];
	float g = colorPixels[idx + 1];
	float b = colorPixels[idx + 2];

	uint meshType = get_mesh_type(r, g, b);

	atomicAdd(colorIndices[meshType], 1);
}