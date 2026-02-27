#ifndef BVH_H
#define BVH_H

#include <stddef.h>
#include "scene.h"

typedef struct {
    aabb box;
    int left;
    int right;
    size_t start;
    size_t count;
} bvh_node;

typedef struct {
    bvh_node *nodes;
    size_t node_count;
    size_t *triangle_indices;
    size_t triangle_count;
    const scene *scene_ref;
} bvh;

int bvh_build(bvh *tree, const scene *s);
void bvh_destroy(bvh *tree);
int bvh_trace_first_hit(const bvh *tree, ray r, float tmin, float tmax, size_t *out_mesh, size_t *out_tri, float *out_t, vec3 *out_normal, float *out_u, float *out_v);

#endif
