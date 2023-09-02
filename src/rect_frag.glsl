#version 450 core

layout(location = 3) uniform vec4 Color;

in vec2 uv;
in vec2 line_thickness;

out vec4 color;

void
main()
{
	vec2 beta = abs(uv - 0.5) - 0.5 + line_thickness;
	color = Color;
	if (beta.x < 0 && beta.y < 0) color = vec4(0);
}
