#version 450 core

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform image2D DisplayTexture;

layout(location = 0) uniform vec2 Resolution;
layout(location = 1) uniform uint FrameIndex;

#define PI          3.14159265358979323
#define ONE_OVER_PI 0.31830988618379067
#define TAU         6.28318530717958647

struct pcg32_state
{
	uint state;
	uint increment;
};

void pcg32_seed(inout pcg32_state pcg_state, uint seed, uint increment);
uint pcg32_next(inout pcg32_state pcg_state);

pcg32_state pcg_state;

float
Random01()
{
	return pcg32_next(pcg_state) * 2.3283064365386962890625e-10;
}

vec3
RandomDir()
{
	float u   = 2*Random01() - 1;
	float phi = TAU*Random01(); 

	float eta = sqrt(1 - u*u);
	return vec3(eta*sin(phi), eta*cos(phi), u);
}

vec3
RandomDirInHemi(vec3 normal)
{
	vec3 dir = RandomDir();
	return (dot(dir, normal) < 0 ? -dir : dir);
}

// TODO: Replace
vec3
CosineWeightedRandomDirInHemi(vec3 plane_normal)
{
	vec3 w;
	{ // NOTE: cosine-weighted hemisphere sampling from: https://alexanderameye.github.io/notes/sampling-the-hemisphere/
		float e0 = Random01();
		float e1 = Random01();

		//float theta = acos(sqrt(e0));
		float cos_theta = sqrt(e0);
		float sin_theta = sqrt(1 - cos_theta*cos_theta);
		float phi   = TAU*e1;

		w = vec3(cos(phi)*sin_theta, sin(phi)*sin_theta, cos_theta);
	}

	vec3 n = plane_normal.yzx; // NOTE: rotate to move singularity below the object
	vec3 b1;
	vec3 b2;
	vec3 v;
	{ // NOTE: orthonormal basis from one vector https://backend.orbit.dtu.dk/ws/portalfiles/portal/126824972/onb_frisvad_jgt2012_v2.pdf
		if (n.z < -0.99999)
		{
			b1 = vec3(0, -1, 0);
			b2 = vec3(-1, 0, 0);
		}
		else
		{
			float a = 1/(1 + n.z);
			float b = -n.x*n.y*a;
			b1 = vec3(1 - n.x*n.x*a, b, -n.x);
			b2 = vec3(b, 1 - n.y*n.y*a, -n.y);
		}

		v = w.x*b1 + w.y*b2 + w.z*n;
	}

	return v.zxy; // NOTE: rotate back
}

struct Hit_Data
{
	bool is_valid;
	uint idx;
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
	//float sqrt_descriminant = sqrt(descriminant);
	//float t0 = b + sqrt_descriminant;
	//float t1 = b - sqrt_descriminant;
	//float t  = min(t0, t1);
	float t = b - sqrt(descriminant);

	Hit_Data result;
	result.is_valid = (descriminant >= 0 && t >= 0);
	result.idx      = 0; // TODO: Yuck
	result.dist     = t;
	result.point    = t*ray + origin;
	result.normal   = normalize(result.point - sphere_pos);

	return result;
}

struct Sphere
{
	vec3 pos;
	float r;
	vec3 color;
	vec3 emission;
	bool is_reflective;
};

float BoxHeight = 20;
float BoxWidth  = 20;
float BoxDepth  = 60;

Sphere Spheres[] = Sphere[](
	Sphere(
		vec3(0, 10000 + BoxHeight/2, -BoxDepth/2),
		10000,
		vec3(1, 1, 1),
		vec3(0),
		false
	),
	Sphere(
		vec3(0, -10000 - BoxHeight/2, -BoxDepth/2),
		10000,
		vec3(1, 1, 1),
		vec3(0),
		false
	),
	Sphere(
		vec3(10000 + BoxWidth/2, 0, -BoxDepth/2),
		10000,
		vec3(1, 0, 0),
		vec3(0),
		false
	),
	Sphere(
		vec3(-10000 - BoxWidth/2, 0, -BoxDepth/2),
		10000,
		vec3(0, 1, 0),
		vec3(0),
		false
	),
	Sphere(
		vec3(0, 0, 10000 + BoxDepth/2),
		10000,
		vec3(1, 1, 1),
		vec3(0),
		false
	),
	Sphere(
		vec3(0, 0, -10000 - BoxDepth/2),
		10000,
		vec3(1, 1, 1),
		vec3(0),
		false
	),
	Sphere(
		vec3(BoxWidth/2 - 4.1, -BoxHeight/2 + 4, -BoxDepth/2 + 4.1),
		4,
		vec3(1, 1, 1),
		vec3(0),
		true
	),
	Sphere(
		vec3(-BoxWidth/2 + 4.1, -BoxHeight/2 + 4, -BoxDepth/2 + 4*3.1),
		4,
		vec3(1, 1, 1),
		vec3(0),
		true
	),
	Sphere(
		vec3(0, BoxHeight/2-4.5, -BoxDepth/2 + 6),
		3,
		vec3(1, 1, 1),
		vec3(1, 1, 1),
		false
	)
);

uint Lights[] = uint[](Spheres.length()-1);

Hit_Data
CastRay(vec3 origin, vec3 ray)
{
	Hit_Data hit = Hit_Data(false, 0, 1e10, vec3(0), vec3(0));
	for (uint i = 0; i < Spheres.length(); ++i)
	{
		Hit_Data new_hit = RaySphere(origin, ray, Spheres[i].pos, Spheres[i].r);
		if (new_hit.is_valid && new_hit.dist < hit.dist)
		{
			hit     = new_hit;
			hit.idx = i; // TODO: Yuck
		}
	}

	return hit;
}

void
main()
{
	uint invocation_index = gl_GlobalInvocationID.y*gl_NumWorkGroups.x*gl_WorkGroupSize.x + gl_GlobalInvocationID.x;
	uint seed             = (invocation_index + FrameIndex*187272781)*178525871;
	pcg32_seed(pcg_state, seed, invocation_index);

	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
	if (pos.x >= Resolution.x || pos.y >= Resolution.y) return;

	vec2 uv = pos/Resolution;

	float fov = 1.57079632679489;

	vec2 image_plane_dim = vec2(1, Resolution.y/Resolution.x);
	float focal_len      = 1/(2*tan(fov/2));

	vec3 origin = vec3(0);
	
	vec3 image_plane_origin = vec3(-image_plane_dim/2, -focal_len);
	vec3 ray                = normalize(image_plane_origin + vec3(image_plane_dim*uv, 0));

	bool use_di = (gl_GlobalInvocationID.x >= (gl_NumWorkGroups.x*gl_WorkGroupSize.x)/2);
	//bool use_di = ((gl_GlobalInvocationID.y/100)%2 == 0);
	use_di = true;

	vec3 color               = vec3(0);
	vec3 multiplier          = vec3(1);
	bool last_was_reflective = false;
	for (uint bounce = 0; bounce < 100; ++bounce)
	{
		Hit_Data hit = CastRay(origin, ray);

		vec3 new_origin = hit.point + ray*0.001; // TODO: Hack to avoid self intersection

		if (!hit.is_valid) break;
		else
		{
			if (Spheres[hit.idx].emission != vec3(0))
			{
				if (bounce == 0 || last_was_reflective || !use_di) color += multiplier*Spheres[hit.idx].emission;
				break;
			}
			else if (Spheres[hit.idx].is_reflective)
			{
				origin = new_origin;
				ray    = reflect(ray, hit.normal);
				last_was_reflective = true;
			}
			else
			{
				uint light_idx = min(Lights.length(), uint(Random01()*Lights.length()));
				uint light_sphere_idx = Lights[light_idx];

				vec3 to_light   = Spheres[light_sphere_idx].pos - hit.point;
				vec3 to_light_n = normalize(to_light);

				Hit_Data shadow_hit = CastRay(new_origin, to_light_n);
				if (shadow_hit.is_valid && use_di)
				{
					vec3 brdf  = Spheres[hit.idx].color*ONE_OVER_PI;
					vec3 light = Spheres[light_sphere_idx].emission;
					float geom_coupling = (dot(to_light_n, hit.normal)*dot(-to_light_n, shadow_hit.normal))/dot(to_light, to_light);
					float res_pdf = TAU*Spheres[light_sphere_idx].r*Spheres[light_sphere_idx].r;

					color += multiplier*brdf*light*geom_coupling*res_pdf;
				}

				multiplier *= Spheres[hit.idx].color;

				origin = new_origin;
				ray = CosineWeightedRandomDirInHemi(hit.normal);
				last_was_reflective = false;
			}
		}
	}

	vec4 old_val = imageLoad(DisplayTexture, pos);
	vec3 new_color = old_val.rgb + (color - old_val.rgb)/(FrameIndex+1);

	imageStore(DisplayTexture, pos, vec4(new_color, 1));
}
