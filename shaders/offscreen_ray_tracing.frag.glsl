#version 450

layout(binding = 1) uniform sampler2D texture_sampler;

layout(location = 0) in vec2 in_tex_coord;
layout(location = 1) in vec2 in_resolution;
layout(location = 2) in vec2 in_mouse_position;
layout(location = 3) in float in_time;

layout(location = 0) out vec4 out_color;

#define EPSILON 1e-4
#define INFINITY 1e6
#define SQRT_3 1.7320508075688772
#define PI 3.141592653589793

struct Ray
{
    vec2 origin;
    vec2 direction;
};

struct Circle
{
    vec2 center;
    float radius;
};

float intersect(const Ray ray, const Circle circle)
{
    const vec2 oc = circle.center - ray.origin;
    const float oc_dot_d = dot(oc, ray.direction);
    const float det = oc_dot_d * oc_dot_d - dot(oc, oc) + circle.radius * circle.radius;
    if (det < 0.0)
    {
        return 0.0;
    }
    const float sqrt_det = sqrt(det);
    const float t1 = oc_dot_d - sqrt_det;
    if (t1 > EPSILON)
    {
        return t1;
    }
    const float t2 = oc_dot_d + sqrt_det;
    if (t2 > EPSILON)
    {
        return t2;
    }
    return 0.0;
}

    #define DIFFUSE 0
    #define SPECULAR 1
    #define REFRACTIVE 2

struct Material
{
    vec3 emission;
    vec3 color;
    int type;
};

struct Physical_circle
{
    Circle circle;
    Material material;
};

const float side = 160.0;

const vec3 red = { 0.75, 0.25, 0.25 };
const vec3 green = { 0.25, 0.75, 0.25 };
const vec3 blue = { 0.25, 0.25, 0.75 };
const vec3 white = { 0.75, 0.75, 0.75 };

const Material white_diffuse = { vec3(0.0), white, DIFFUSE };
const Material red_diffuse = { vec3(0.0), red, DIFFUSE };
const Material blue_diffuse = { vec3(0.0), blue, DIFFUSE };
const Material green_diffuse = { vec3(0.0), green, DIFFUSE };
const Material white_specular = { vec3(0.0), white, SPECULAR };
const Material white_refractive = { vec3(0.0), white, REFRACTIVE };
const Material white_light = { { 10, 10, 10 }, white, DIFFUSE };

#define NUM_CIRCLES 4
Physical_circle circles[NUM_CIRCLES] =
{
{ { { side * 0.3, side * 0.3 }, side * 0.01 }, white_light },
{ { { side * 0.5, side * 0.35 }, side * 0.07 }, white_refractive },
{ { { side * 0.3, side * 0.5 }, side * 0.12 }, white_refractive },
{ { { side * 0.5, side * 0.75 }, side * 0.09 }, white_refractive }
};

struct Hit
{
    int index;
    float t;
};

#define RANDOM_A 48271
#define RANDOM_M 2147483647
int next_random_value(const int rng_state)
{
    return (rng_state * RANDOM_A) % RANDOM_M;
}

float random_unit(inout int rng_state)
{
    rng_state = next_random_value(rng_state);
    return float(rng_state) / float(RANDOM_M);
}

vec2 sample_circle(inout int rng_state)
{
    vec2 result;
    float norm_squared;
    do
    {
        result.x = random_unit(rng_state) * 2.0 - 1.0;
        result.y = random_unit(rng_state) * 2.0 - 1.0;
        norm_squared = dot(result, result);
    } while (norm_squared == 0.0 || norm_squared > 1.0);
    return result / sqrt(norm_squared);
}

vec2 sample_semicircle(inout int rng_state)
{
    vec2 result;
    float norm_squared;
    do
    {
        result.x = random_unit(rng_state) * 2.0 - 1.0;
        result.y = random_unit(rng_state);
        norm_squared = dot(result, result);
    } while (norm_squared == 0.0 || norm_squared > 1.0);
    return result / sqrt(norm_squared);
}

vec2 diffuse(const vec2 normal, inout int rng_state)
{
    const vec2 sampled = sample_semicircle(rng_state);
    const vec2 parallel = { normal.y, -normal.x };
    return sampled.x * parallel + sampled.y * normal;
}

Hit intersect(const Ray ray)
{
    float t_min = INFINITY;
    int index = 0;

    for (int i = 0; i < NUM_CIRCLES; ++i)
    {
        const float t = intersect(ray, circles[i].circle);
        if (t > 0.0 && t < t_min)
        {
            t_min = t;
            index = i;
        }
    }

    if (t_min < INFINITY)
    {
        const Hit result = { index, t_min };
        return result;
    }

    const Hit result = { 0, 0.0 };
    return result;
}

vec3 radiance(const Ray ray, inout int rng_state)
{
    vec3 accumulated_color = { 0, 0, 0 };
    vec3 accumulated_reflectance=  { 1, 1, 1 };
    Ray r = ray;
    int depth = 0;
    const int min_depth = 5;

    while (true)
    {
        const Hit hit = intersect(r);
        if (hit.t <= 0.0)
        {
            return accumulated_color;
        }

        const vec2 hit_point = r.origin + hit.t * r.direction;
        bool into = false;// from outside going inside
        vec2 normal = vec2(0.0);
        Material hit_material = { vec3(0.0), vec3(0.0), 0 };

        const Physical_circle circle = circles[hit.index];
        const vec2 exterior_normal = normalize(hit_point - circle.circle.center);
        into = dot(exterior_normal, r.direction) < 0.0;
        normal = into ? exterior_normal : -exterior_normal;
        hit_material = circle.material;

        accumulated_color += accumulated_reflectance * hit_material.emission;

        vec3 color = hit_material.color;
        if (depth > min_depth)
        {
            const float p =  max(max(color.x, color.y), color.z);
            if (random_unit(rng_state) < p)
            {
                color /= p;
            }
            else
            {
                return accumulated_color;
            }
        }
        accumulated_reflectance *= color;

        r.origin = hit_point;

        switch (hit_material.type)
        {
            case DIFFUSE:
            {
                r.direction = diffuse(normal, rng_state);
                break;
            }
            case SPECULAR:
            {
                r.direction = reflect(r.direction, normal);
                break;
            }
            case REFRACTIVE:
            {
                const vec2 reflected_direction = reflect(r.direction, normal);
                const float n_air = 1.0;
                const float n_glass = 1.5;
                const float eta = into ? n_air / n_glass : n_glass / n_air;
                const float incident_dot_normal = dot(r.direction, normal);
                const float cos2t = 1.0 - eta * eta * (1.0 - incident_dot_normal * incident_dot_normal);

                if (cos2t < 0.0)
                {
                    // Total internal reflection
                    r.direction = reflected_direction;
                    break;
                }

                const vec2 transmitted_direction = normalize(r.direction * eta - normal * (incident_dot_normal * eta +  sqrt(cos2t)));
                const float a = n_glass - n_air;
                const float b = n_glass + n_air;
                const float R0 = a * a / (b * b);
                const float c = into ? 1.0 + incident_dot_normal : 1.0 + dot(transmitted_direction, normal);
                const float Re = R0 + (1.0 - R0) * c * c * c * c * c;
                const float Tr = 1.0 - Re;
                const float P = 0.25 + 0.5 * Re;
                const float RP = Re / P;
                const float TP = Tr / (1.0 - P);

                if (random_unit(rng_state) < P)
                {
                    accumulated_reflectance *= RP;
                    r.direction = reflected_direction;
                }
                else
                {
                    accumulated_reflectance *= TP;
                    r.direction = transmitted_direction;
                }
                break;
            }
        }

        ++depth;
    }

    return vec3(0.0);
}

    #define NUM_SAMPLES 100

void main()
{
    int rng_state = int(gl_FragCoord.y * in_resolution.x + gl_FragCoord.x) * 48271;
    rng_state = next_random_value(rng_state);
    rng_state = next_random_value(rng_state);
    rng_state = next_random_value(rng_state);
    rng_state = next_random_value(rng_state);

    circles[0].circle.center = in_mouse_position;

    vec3 accumulated_color = vec3(0.0);
    for(int s = 0; s < NUM_SAMPLES; ++s)
    {
        const vec2 position = gl_FragCoord.xy;
        const vec2 direction = sample_circle(rng_state);
        const Ray ray = { position, direction };
        accumulated_color += radiance(ray, rng_state);
    }
    const vec3 color = accumulated_color / float(NUM_SAMPLES);

    out_color = vec4(color, 0.0);
}