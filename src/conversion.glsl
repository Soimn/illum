#version 450 core

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform readonly image2D SumTexture;
layout(rgba32f, binding = 1) uniform readonly image2D CompensationTexture;
layout(rgba32f, binding = 2) uniform writeonly image2D DisplayTexture;

layout(location = 0) uniform vec2 Resolution;

void
main()
{
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
	if (pos.x >= Resolution.x || pos.y >= Resolution.y) return;

	vec4 sum = imageLoad(SumTexture, pos);
	vec3 comp = imageLoad(CompensationTexture, pos).rgb;
	vec3 color = (sum.rgb + comp.rgb)/sum.a;
	if (gl_GlobalInvocationID.x >= (gl_NumWorkGroups.x*gl_WorkGroupSize.x)/2) color = sum.rgb/sum.a;
	imageStore(DisplayTexture, pos, vec4(color, 1));
}
