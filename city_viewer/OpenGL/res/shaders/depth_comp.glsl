#version 430

layout(std430, binding = 0) buffer DepthBuffer {
	float depthPixels[];
};

layout(std430, binding = 1) buffer ResultBuffer {
	//     0          1            2
	// [min_depth, max_depth, depth_sum]
	float results[3];
};

uniform uint depth_len;
uniform uint depth_num_channels;

void main() {
	uint idx = gl_GlobalInvocationID.x;
	if (idx >= depth_len) {
		return;
	}

	float depth = depthPixels[idx * depth_num_channels];

	atomicMin(results[0], depth);
	atomicMax(results[1], depth);
	atomicAdd(results[2], depth);
}