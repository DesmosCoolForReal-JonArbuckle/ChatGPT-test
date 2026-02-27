#ifndef MATH3D_H
#define MATH3D_H

#include <math.h>

typedef struct { float x, y, z; } vec3;

typedef struct { vec3 min, max; } aabb;

typedef struct {
    vec3 origin;
    vec3 direction;
} ray;

static inline vec3 vec3_add(vec3 a, vec3 b) { return (vec3){a.x+b.x,a.y+b.y,a.z+b.z}; }
static inline vec3 vec3_sub(vec3 a, vec3 b) { return (vec3){a.x-b.x,a.y-b.y,a.z-b.z}; }
static inline vec3 vec3_mul(vec3 a, float s) { return (vec3){a.x*s,a.y*s,a.z*s}; }
static inline float vec3_dot(vec3 a, vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline vec3 vec3_cross(vec3 a, vec3 b) {
    return (vec3){a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
}
static inline float vec3_len(vec3 a) { return sqrtf(vec3_dot(a,a)); }
static inline vec3 vec3_norm(vec3 a) {
    float l = vec3_len(a);
    return l > 0.0f ? vec3_mul(a,1.0f/l) : (vec3){0.0f,0.0f,0.0f};
}

#endif
