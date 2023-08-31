#version 450 core

out gl_PerVertex { vec4 gl_Position; };
out vec2 uv;

void
main()
{
  vec2 a = vec2(gl_VertexID%2, gl_VertexID/2);
  gl_Position = vec4(a*4 - 1, 0, 1);
  uv = a*2;
}
