#version 450 core

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform image2D DisplayTexture;

layout(location = 0) uniform vec2 Resolution;

struct Hit_Data
{
	bool is_valid;
	float dist;
	vec3 point;
	vec3 normal;
};

Hit_Data
RaySphere(vec3 origin, vec3 ray, vec3 sphere_pos, float sphere_r)
{
	vec3 p  = sphere_pos - origin;
	float b = dot(ray, p);
	float descriminant = b*b + (sphere_r*sphere_r - dot(p, p));
	float t = b - sqrt(descriminant);

	Hit_Data result;
	result.is_valid = (descriminant >= 0);
	result.dist     = t;
	result.point    = t*ray + origin;
	result.normal   = normalize(result.point - sphere_pos);

	return result;
}

void
main()
{
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
	if (pos.x >= Resolution.x || pos.y >= Resolution.y) return;

	vec2 uv = pos/Resolution;

	vec3 color = vec3(0);

	vec3 light = vec3(-0.4, 1, 0.3);

	vec3 sphere_pos = vec3(0, 0, -5);
	float sphere_r = 1;

	float fov = 1.57079632679489;

	vec2 image_plane_dim = vec2(1, Resolution.y/Resolution.x);
	float focal_len      = 1/(2*tan(fov/2));

	vec3 origin = vec3(0);
	
	vec3 image_plane_origin = vec3(-image_plane_dim/2, -focal_len);
	vec3 ray                = normalize(image_plane_origin + vec3(image_plane_dim*uv, 0));

	Hit_Data hit = RaySphere(origin, ray, sphere_pos, sphere_r);
	if (hit.is_valid)
	{
		color = min(vec3(1), 0.2 + max(dot(hit.normal, light), 0)*vec3(1));
	}

	imageStore(DisplayTexture, pos, vec4(color, 1));
}
