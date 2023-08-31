#version 450 core

layout(location = 0) uniform vec2 Resolution;

uniform sampler2D DisplayTexture;

in vec2 uv;

out vec4 color;

void
main()
{
  color = vec4(texture(DisplayTexture, uv).rgb, 1);
}
