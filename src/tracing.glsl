#version 450 core

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform image2D DisplayTexture;

layout(location = 0) uniform vec2 Resolution;

void
main()
{
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
	pos.x %= int(Resolution.x);
	pos.y %= int(Resolution.y);

	imageStore(DisplayTexture, pos, vec4(vec3(((pos.x/10 % 2) + (pos.y/10 % 2)) % 2), 1));
}
