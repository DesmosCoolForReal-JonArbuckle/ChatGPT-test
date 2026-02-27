#include "bvh.h"

#include <float.h>
#include <stdlib.h>
#include <string.h>

static aabb aabb_empty(void) {
    return (aabb){{FLT_MAX, FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX, -FLT_MAX}};
}

static void aabb_include(aabb *b, vec3 p) {
    if (p.x < b->min.x) b->min.x = p.x;
    if (p.y < b->min.y) b->min.y = p.y;
    if (p.z < b->min.z) b->min.z = p.z;
    if (p.x > b->max.x) b->max.x = p.x;
    if (p.y > b->max.y) b->max.y = p.y;
    if (p.z > b->max.z) b->max.z = p.z;
}

static int intersect_aabb(ray r, aabb b, float tmin, float tmax) {
    for (int axis = 0; axis < 3; ++axis) {
        float origin = axis == 0 ? r.origin.x : axis == 1 ? r.origin.y : r.origin.z;
        float dir = axis == 0 ? r.direction.x : axis == 1 ? r.direction.y : r.direction.z;
        float minv = axis == 0 ? b.min.x : axis == 1 ? b.min.y : b.min.z;
        float maxv = axis == 0 ? b.max.x : axis == 1 ? b.max.y : b.max.z;
        float invd = 1.0f / dir;
        float t0 = (minv - origin) * invd;
        float t1 = (maxv - origin) * invd;
        if (invd < 0.0f) {
            float tmp = t0;
            t0 = t1;
            t1 = tmp;
        }
        if (t0 > tmin) tmin = t0;
        if (t1 < tmax) tmax = t1;
        if (tmax <= tmin) return 0;
    }
    return 1;
}

static int intersect_triangle(ray r, vec3 v0, vec3 v1, vec3 v2, float *t, float *u, float *v) {
    const float eps = 1e-6f;
    vec3 e1 = vec3_sub(v1, v0);
    vec3 e2 = vec3_sub(v2, v0);
    vec3 p = vec3_cross(r.direction, e2);
    float det = vec3_dot(e1, p);
    if (det > -eps && det < eps) return 0;
    float inv_det = 1.0f / det;
    vec3 s = vec3_sub(r.origin, v0);
    *u = inv_det * vec3_dot(s, p);
    if (*u < 0.0f || *u > 1.0f) return 0;
    vec3 q = vec3_cross(s, e1);
    *v = inv_det * vec3_dot(r.direction, q);
    if (*v < 0.0f || (*u + *v) > 1.0f) return 0;
    *t = inv_det * vec3_dot(e2, q);
    return *t > eps;
}

int bvh_build(bvh *tree, const scene *s) {
    memset(tree, 0, sizeof(*tree));
    tree->scene_ref = s;

    size_t tri_total = 0;
    for (size_t i = 0; i < s->mesh_count; ++i) tri_total += s->meshes[i].triangle_count;
    tree->triangle_count = tri_total;
    tree->triangle_indices = (size_t*)calloc(tri_total, sizeof(size_t));
    tree->nodes = (bvh_node*)calloc(1, sizeof(bvh_node));
    tree->node_count = 1;
    if (!tree->triangle_indices || !tree->nodes) {
        bvh_destroy(tree);
        return 0;
    }

    for (size_t i = 0; i < tri_total; ++i) tree->triangle_indices[i] = i;
    tree->nodes[0].left = -1;
    tree->nodes[0].right = -1;
    tree->nodes[0].start = 0;
    tree->nodes[0].count = tri_total;

    aabb box = aabb_empty();
    for (size_t m = 0; m < s->mesh_count; ++m) {
        for (size_t v = 0; v < s->meshes[m].vertex_count; ++v) {
            aabb_include(&box, s->meshes[m].vertices[v].position);
        }
    }
    tree->nodes[0].box = box;
    return 1;
}

void bvh_destroy(bvh *tree) {
    if (!tree) return;
    free(tree->nodes);
    free(tree->triangle_indices);
    memset(tree, 0, sizeof(*tree));
}

int bvh_trace_first_hit(const bvh *tree, ray r, float tmin, float tmax, size_t *out_mesh, size_t *out_tri, float *out_t, vec3 *out_normal, float *out_u, float *out_v) {
    if (!tree || !tree->nodes || !tree->scene_ref) return 0;
    const scene *s = tree->scene_ref;
    if (!intersect_aabb(r, tree->nodes[0].box, tmin, tmax)) return 0;

    int hit = 0;
    size_t global_idx = 0;
    float best_t = tmax;

    for (size_t m = 0; m < s->mesh_count; ++m) {
        const mesh *me = &s->meshes[m];
        for (size_t t = 0; t < me->triangle_count; ++t, ++global_idx) {
            (void)global_idx;
            triangle tri = me->triangles[t];
            vec3 v0 = me->vertices[tri.i0].position;
            vec3 v1 = me->vertices[tri.i1].position;
            vec3 v2 = me->vertices[tri.i2].position;
            float tt, uu, vv;
            if (intersect_triangle(r, v0, v1, v2, &tt, &uu, &vv) && tt < best_t && tt > tmin) {
                best_t = tt;
                if (out_mesh) *out_mesh = m;
                if (out_tri) *out_tri = t;
                if (out_t) *out_t = tt;
                if (out_u) *out_u = uu;
                if (out_v) *out_v = vv;
                if (out_normal) {
                    vec3 n0 = me->vertices[tri.i0].normal;
                    vec3 n1 = me->vertices[tri.i1].normal;
                    vec3 n2 = me->vertices[tri.i2].normal;
                    float w = 1.0f - uu - vv;
                    *out_normal = vec3_norm(vec3_add(vec3_add(vec3_mul(n0, w), vec3_mul(n1, uu)), vec3_mul(n2, vv)));
                }
                hit = 1;
            }
        }
    }

    return hit;
}
