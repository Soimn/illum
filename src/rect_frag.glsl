#version 450 core

layout(location = 3) uniform vec4 Color;

in vec2 uv;
in vec2 line_thickness;

out vec4 color;

void
main()
{
	vec2 beta = -sign(abs(uv - 0.5) - 0.5 + line_thickness);
	float alpha = 1 - max(0, beta.x + beta.y)/2;
	color = Color*alpha;
}
