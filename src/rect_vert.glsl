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

	vec2 centered_vertex = vertex - 0.5;
	vec2 scaled_vertex   = centered_vertex*vec2(1, PosDim.w/PosDim.z);

	float cos_theta = cos(ZRotLine.y);
	float sin_theta = sin(ZRotLine.y);
	vec2 rotated_vertex = mat2(cos_theta, sin_theta, -sin_theta, cos_theta)*scaled_vertex;

	float aspect_ratio = Resolution.x/Resolution.y;
	vec2 acorrected_vertex = rotated_vertex*vec2(1, aspect_ratio);

	gl_Position    = vec4(acorrected_vertex, ZRotLine.x, 1);
	uv             = vertex;
	line_thickness = vec2(ZRotLine.z/PosDim.x, ZRotLine.z/PosDim.y * aspect_ratio);
}
