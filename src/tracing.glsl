#version 450 core

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform image2D SumTexture;
layout(rgba32f, binding = 1) uniform image2D CompensationTexture;

layout(location = 0) uniform vec2 Resolution;
layout(location = 1) uniform uint FrameIndex;

#define PI          3.14159265358979323
#define ONE_OVER_PI 0.31830988618379067
#define TAU         6.28318530717958647
#define PI_OVER_4   0.78539816339744830

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
RaySphere(vec3 origin, vec3 ray, vec3 sphere_pos, float sphere_r, bool invert_faces)
{
	vec3 p  = sphere_pos - origin;
	float b = dot(ray, p);
	float descriminant = b*b + (sphere_r*sphere_r - dot(p, p));
	float sqrt_descriminant = sqrt(descriminant);
	float t0 = b + sqrt_descriminant;
	float t1 = b - sqrt_descriminant;
	float t = (invert_faces ? t0 : t1);

	vec3 point    = t*ray + origin;
	vec3 normal   = normalize(point - sphere_pos);

	Hit_Data result;
	result.is_valid = (descriminant >= 0 && t >= 0);
	result.idx      = 0; // TODO: Yuck
	result.dist     = t;
	result.point    = point;
	result.normal   = (invert_faces ? -normal : normal);

	return result;
}

#define MATERIAL_LIGHT      0
#define MATERIAL_DIFFUSE    1
#define MATERIAL_REFLECTIVE 2
#define MATERIAL_REFRACTIVE 3

struct Sphere
{
	vec3 pos;
	float r;
	vec3 color;
	uint material;
};

float BoxHeight = 20;
float BoxWidth  = 20;
float BoxDepth  = 60;

Sphere Spheres[] = Sphere[](
	Sphere(
		vec3(0, 10000 + BoxHeight/2, -BoxDepth/2),
		10000,
		vec3(1, 1, 1),
		MATERIAL_DIFFUSE
	),
	Sphere(
		vec3(0, -10000 - BoxHeight/2, -BoxDepth/2),
		10000,
		vec3(1, 1, 1),
		MATERIAL_DIFFUSE
	),
	Sphere(
		vec3(10000 + BoxWidth/2, 0, -BoxDepth/2),
		10000,
		vec3(1, 0, 0),
		MATERIAL_DIFFUSE
	),
	Sphere(
		vec3(-10000 - BoxWidth/2, 0, -BoxDepth/2),
		10000,
		vec3(0, 1, 0),
		MATERIAL_DIFFUSE
	),
	Sphere(
		vec3(0, 0, 10000 + BoxDepth/2),
		10000,
		vec3(1, 1, 1),
		MATERIAL_DIFFUSE
	),
	Sphere(
		vec3(0, 0, -10000 - BoxDepth/2),
		10000,
		vec3(1, 1, 1),
		MATERIAL_DIFFUSE
	),
	Sphere(
		vec3(BoxWidth/2 - 4.1, -BoxHeight/2 + 6, -BoxDepth/2 + 4.1),
		4,
		vec3(1.769, 0, 0),
		MATERIAL_REFRACTIVE
	),
	Sphere(
		vec3(-BoxWidth/2 + 4.1, -BoxHeight/2 + 4, -BoxDepth/2 + 12.4),
		4,
		vec3(1, 1, 1),
		MATERIAL_REFLECTIVE
	),
	Sphere(
		vec3(0, BoxHeight/2-4.5, -BoxDepth/2 + 6),
		1,
		vec3(1, 1, 1)*20,
		MATERIAL_LIGHT
	)
);

uint Lights[] = uint[](Spheres.length()-1);

Hit_Data
CastRay(vec3 origin, vec3 ray, bool is_transmitted_ray)
{
	Hit_Data hit = Hit_Data(false, 0, 1e10, vec3(0), vec3(0));
	for (uint i = 0; i < Spheres.length(); ++i)
	{
		Hit_Data new_hit = RaySphere(origin, ray, Spheres[i].pos, Spheres[i].r, is_transmitted_ray);
		if (new_hit.is_valid && new_hit.dist < hit.dist)
		{
			hit     = new_hit;
			hit.idx = i; // TODO: Yuck
		}
	}

	return hit;
}

#define AIR_IOR 1.000293

float
Fresnel(vec3 ray, vec3 normal, float n1, float n2)
{
	// NOTE: https://en.wikipedia.org/wiki/Schlick%27s_approximation
	//       https://blog.demofox.org/2017/01/09/raytracing-reflection-refraction-fresnel-total-internal-reflection-and-beers-law/
	//       https://graphics.stanford.edu/courses/cs148-10-summer/docs/2006--degreve--reflection_refraction.pdf
	
	float n              = n1/n2;
	float cos_theta_i    = dot(-ray, normal);
	float sin_sq_theta_t = n*n*(1 - cos_theta_i*cos_theta_i);
	float cos_theta_t    = sqrt(1 - sin_sq_theta_t);

	float r = 1;
	if (sin_sq_theta_t <= 1)
	{
		float r_s = (n1*cos_theta_i - n2*cos_theta_t)/(n1*cos_theta_i + n2*cos_theta_t);
		float r_p = (n2*cos_theta_i - n1*cos_theta_t)/(n2*cos_theta_i + n1*cos_theta_t);

		r = (r_s*r_s + r_p*r_p)/2;
	}

	return r;
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

	float half_fov = PI_OVER_4;

	vec2 image_plane_dim = vec2(1, Resolution.y/Resolution.x);
	float focal_len      = 1/(2*tan(half_fov));

	vec3 origin = vec3(0);
	
	vec3 image_plane_origin = vec3(-image_plane_dim/2, -focal_len);
	vec3 ray                = normalize(image_plane_origin + vec3(image_plane_dim*uv, 0));

	bool use_di = (gl_GlobalInvocationID.x >= (gl_NumWorkGroups.x*gl_WorkGroupSize.x)/2);
	//bool use_di = ((gl_GlobalInvocationID.y/100)%2 == 0);
	use_di = true;

	vec3 color              = vec3(0);
	vec3 multiplier         = vec3(1);
	bool last_was_diffuse   = false;
	bool is_transmitted_ray = false;
	for (uint bounce = 0; bounce < 100; ++bounce)
	{
		Hit_Data hit = CastRay(origin, ray, is_transmitted_ray);

		vec3 new_origin = hit.point + ray*0.001; // TODO: Hack to avoid self intersection

		if (!hit.is_valid) break;
		else
		{
			if (Spheres[hit.idx].material == MATERIAL_LIGHT)
			{
				if (bounce == 0 || !last_was_diffuse || !use_di) color += multiplier*Spheres[hit.idx].color;
				break;
			}
			else if (Spheres[hit.idx].material == MATERIAL_REFLECTIVE)
			{
				origin           = new_origin;
				ray              = reflect(ray, hit.normal);
				last_was_diffuse = false;
			}
			else if (Spheres[hit.idx].material == MATERIAL_REFRACTIVE)
			{
				float ior = Spheres[hit.idx].color.r;

				float n1 = AIR_IOR;
				float n2 = ior;
				if (is_transmitted_ray)
				{
					n1 = ior;
					n2 = AIR_IOR;
				}

				if (Random01() <= Fresnel(ray, hit.normal, n1, n2))
				{
					origin             = new_origin;
					ray                = reflect(ray, hit.normal);
					last_was_diffuse   = false;
					is_transmitted_ray = is_transmitted_ray;
				}
				else
				{
					origin             = new_origin;
					ray                = refract(ray, hit.normal, n1/n2);
					last_was_diffuse   = false;
					is_transmitted_ray = !is_transmitted_ray;
				}
			}
			else
			{
				uint light_idx = min(Lights.length(), uint(Random01()*Lights.length()));
				uint light_sphere_idx = Lights[light_idx];

				vec3 to_light   = Spheres[light_sphere_idx].pos - hit.point;
				vec3 to_light_n = normalize(to_light);

				Hit_Data shadow_hit = CastRay(new_origin, to_light_n, false);
				if (shadow_hit.is_valid && shadow_hit.idx == light_sphere_idx && use_di)
				{
					vec3 brdf  = Spheres[hit.idx].color*ONE_OVER_PI;
					vec3 light = Spheres[light_sphere_idx].color;
					float geom_coupling = (dot(to_light_n, hit.normal)*dot(-to_light_n, shadow_hit.normal))/dot(to_light, to_light);
					float res_pdf = PI*Spheres[light_sphere_idx].r*Spheres[light_sphere_idx].r;

					color += multiplier*brdf*light*geom_coupling*res_pdf;
				}

				multiplier *= Spheres[hit.idx].color;

				origin             = new_origin;
				ray                = CosineWeightedRandomDirInHemi(hit.normal);
				last_was_diffuse   = true;
				is_transmitted_ray = false;
			}
		}
	}

	// NOTE: KahanBabushNeumaierSum https://en.wikipedia.org/wiki/Kahan_summation_algorithm
	vec3 old_sum  = imageLoad(SumTexture, pos).rgb;
	vec3 old_comp = imageLoad(CompensationTexture, pos).rgb; 

	vec3 new_sum  = old_sum + color;
	vec3 new_comp = old_comp + vec3(
		old_sum.x >= color.x ? (old_sum.x - new_sum.x) + color.x : (color.x - new_sum.x) + old_sum.x,
		old_sum.y >= color.y ? (old_sum.y - new_sum.y) + color.y : (color.y - new_sum.y) + old_sum.y,
		old_sum.z >= color.z ? (old_sum.z - new_sum.z) + color.z : (color.z - new_sum.z) + old_sum.z
	);

	imageStore(SumTexture, pos, vec4(new_sum, FrameIndex+1));
	imageStore(CompensationTexture, pos, vec4(new_comp, 0));
}
