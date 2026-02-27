#include "software_rt.h"
#include "bvh.h"

#include <stdlib.h>

static vec3 mul(vec3 a, vec3 b) { return (vec3){a.x*b.x,a.y*b.y,a.z*b.z}; }

int render_software(const scene *s, framebuffer *fb) {
    if (!s || !fb || !fb->rgba8 || fb->width == 0 || fb->height == 0) return 0;

    bvh tree;
    if (!bvh_build(&tree, s)) return 0;

    vec3 cam_pos = {0.0f, 0.0f, -3.0f};
    vec3 light_dir = vec3_norm((vec3){1.0f, 1.0f, -1.0f});

    for (uint32_t y = 0; y < fb->height; ++y) {
        for (uint32_t x = 0; x < fb->width; ++x) {
            float ndc_x = ((float)x + 0.5f) / (float)fb->width;
            float ndc_y = ((float)y + 0.5f) / (float)fb->height;
            float px = (2.0f * ndc_x - 1.0f);
            float py = (1.0f - 2.0f * ndc_y);

            ray r = {cam_pos, vec3_norm((vec3){px, py, 1.5f})};
            size_t mesh_idx = 0;
            size_t tri_idx = 0;
            float t = 0.0f;
            vec3 geom_n = {0};
            float bu = 0.0f, bv = 0.0f;
            vec3 color = {0.03f, 0.03f, 0.05f};

            if (bvh_trace_first_hit(&tree, r, 0.001f, 1e30f, &mesh_idx, &tri_idx, &t, &geom_n, &bu, &bv)) {
                const mesh *m = &s->meshes[mesh_idx];
                triangle tr = m->triangles[tri_idx];
                const material *mat = &s->materials[tr.material_index];
                vertex v0 = m->vertices[tr.i0];
                vertex v1 = m->vertices[tr.i1];
                vertex v2 = m->vertices[tr.i2];
                float bw = 1.0f - bu - bv;
                float u = bw * v0.u + bu * v1.u + bv * v2.u;
                float v = bw * v0.v + bu * v1.v + bv * v2.v;

                vec3 albedo = mat->albedo;
                if (mat->albedo_texture >= 0 && (size_t)mat->albedo_texture < s->texture_count) {
                    albedo = mul(albedo, sample_texture(&s->textures[mat->albedo_texture], u, v));
                }

                vec3 mapped_n = geom_n;
                if (mat->normal_texture >= 0 && (size_t)mat->normal_texture < s->texture_count) {
                    vec3 ntex = sample_texture(&s->textures[mat->normal_texture], u, v);
                    mapped_n = vec3_norm((vec3){2.0f * ntex.x - 1.0f, 2.0f * ntex.y - 1.0f, 2.0f * ntex.z - 1.0f});
                }

                float ndotl = vec3_dot(mapped_n, vec3_mul(light_dir, -1.0f));
                if (ndotl < 0.0f) ndotl = 0.0f;
                float ambient = 0.08f;
                float gloss = (1.0f - mat->roughness);
                color = vec3_add(vec3_mul(albedo, ambient + ndotl), (vec3){0.04f*gloss,0.04f*gloss,0.04f*gloss});
            }

            size_t idx = (size_t)(y * fb->width + x) * 4;
            fb->rgba8[idx + 0] = (uint8_t)(fminf(color.x, 1.0f) * 255.0f);
            fb->rgba8[idx + 1] = (uint8_t)(fminf(color.y, 1.0f) * 255.0f);
            fb->rgba8[idx + 2] = (uint8_t)(fminf(color.z, 1.0f) * 255.0f);
            fb->rgba8[idx + 3] = 255;
        }
    }

    bvh_destroy(&tree);
    return 1;
}
