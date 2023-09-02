#version 450 core

layout(location = 0) uniform vec2 Resolution;
layout(location = 1) uniform vec4 PosDim;
layout(location = 2) uniform vec4 ZRotLine;

out gl_PerVertex { vec4 gl_Position; };
out vec2 uv;
out vec2 line_thickness;

void
main()
{
	vec2 vertex = vec2(int(((gl_VertexID + 1) % 6)/3), int(((gl_VertexID + 5) % 6)/3));

	vec2 dim       = 2*PosDim.zw/Resolution;
	vec3 pos       = vec3(2*vec2(PosDim.x, Resolution.y - PosDim.y)/Resolution - 1, ZRotLine.x);
	float rot      = ZRotLine.y;
	vec2 thickness = vec2(ZRotLine.z/PosDim.z, ZRotLine.z/PosDim.w);

	float cos_theta = cos(rot);
	float sin_theta = sin(rot);

	vec2 scaled_vertex  = vertex*dim - dim/2;
	vec2 rotated_vertex = vec2(cos_theta*scaled_vertex.x - sin_theta*scaled_vertex.y, sin_theta*scaled_vertex.x + cos_theta*scaled_vertex.y);

	gl_Position    = vec4(rotated_vertex.x + pos.x, rotated_vertex.y + pos.y, pos.z, 1);
	uv             = vertex.xy;
	line_thickness = thickness;

	// TODO: Fix rotation and z layering
}
