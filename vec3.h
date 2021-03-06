#ifndef VEC3_H
#define VEC3_H

#include <math.h>
#include <stdio.h>

struct vec3 {
    union {
        float e[3];
        struct {
            float x, y, z;
        };
    };
};

static int vec3_print_decimals = 3;
static inline void vec3_print(const char *str, const struct vec3 v)
{
    const int d = vec3_print_decimals;
    printf("%s: { x = %.*f, y = %.*f, z = %.*f }\n", str, d, v.x, d, v.y, d, v.z);
}

static inline struct vec3 vec3_add(const struct vec3 a, const struct vec3 b)
{
    return (struct vec3) { a.x + b.x, a.y + b.y, a.z + b.z };
}

static inline struct vec3 vec3_sub(const struct vec3 a, const struct vec3 b)
{
    return (struct vec3) { a.x - b.x, a.y - b.y, a.z - b.z };
}

static inline float vec3_sum(const struct vec3 a)
{
    return a.x + a.y + a.z;
}

static inline struct vec3 vec3_inv(const struct vec3 a)
{
    return (struct vec3) { 1.0f / a.x, 1.0f / a.y, 1.0f / a.z };
}

static inline struct vec3 vec3_muls(const struct vec3 a, const float b)
{
    return (struct vec3) { a.x * b, a.y * b, a.z * b };
}

static inline struct vec3 vec3_spow2(const struct vec3 a)
{
    return (struct vec3) { a.x * fabs(a.x), a.y * fabs(a.y), a.z * fabs(a.z) };
}

static inline struct vec3 vec3_pow2(const struct vec3 a)
{
    return (struct vec3) { a.x * a.x, a.y * a.y, a.z * a.z };
}

static inline float vec3_norm2(const struct vec3 a)
{
    return a.x * a.x + a.y * a.y + a.z * a.z;
}

static inline float vec3_norm(const struct vec3 a)
{
    return sqrtf(vec3_norm2(a));
}

static inline struct vec3 vec3_normalize(const struct vec3 a)
{
    return vec3_muls(a, 1.0f / vec3_norm(a));
}

static inline float vec3_dot(const struct vec3 a, const struct vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

#endif
