#include "scene.h"

#include <stdlib.h>
#include <string.h>

static uint8_t clamp255(float f) {
    if (f < 0.0f) f = 0.0f;
    if (f > 1.0f) f = 1.0f;
    return (uint8_t)(f * 255.0f + 0.5f);
}

vec3 sample_texture(const texture *tx, float u, float v) {
    if (!tx || !tx->rgba8 || tx->width == 0 || tx->height == 0) {
        return (vec3){1.0f, 1.0f, 1.0f};
    }
    u = u - (int)u;
    v = v - (int)v;
    if (u < 0.0f) u += 1.0f;
    if (v < 0.0f) v += 1.0f;

    uint32_t x = (uint32_t)(u * (float)(tx->width - 1));
    uint32_t y = (uint32_t)(v * (float)(tx->height - 1));
    size_t idx = (size_t)(y * tx->width + x) * 4;
    return (vec3){
        tx->rgba8[idx + 0] / 255.0f,
        tx->rgba8[idx + 1] / 255.0f,
        tx->rgba8[idx + 2] / 255.0f
    };
}

int build_demo_scene(scene *out_scene) {
    memset(out_scene, 0, sizeof(*out_scene));

    out_scene->mesh_count = 1;
    out_scene->meshes = (mesh*)calloc(out_scene->mesh_count, sizeof(mesh));

    out_scene->texture_count = 2;
    out_scene->textures = (texture*)calloc(out_scene->texture_count, sizeof(texture));

    out_scene->material_count = 1;
    out_scene->materials = (material*)calloc(out_scene->material_count, sizeof(material));

    if (!out_scene->meshes || !out_scene->textures || !out_scene->materials) {
        destroy_scene(out_scene);
        return 0;
    }

    mesh *m = &out_scene->meshes[0];
    m->vertex_count = 4;
    m->triangle_count = 2;
    m->vertices = (vertex*)calloc(m->vertex_count, sizeof(vertex));
    m->triangles = (triangle*)calloc(m->triangle_count, sizeof(triangle));
    if (!m->vertices || !m->triangles) {
        destroy_scene(out_scene);
        return 0;
    }

    m->vertices[0] = (vertex){{-1.0f,-1.0f, 0.0f},{0.0f,0.0f,1.0f},0.0f,0.0f};
    m->vertices[1] = (vertex){{ 1.0f,-1.0f, 0.0f},{0.0f,0.0f,1.0f},1.0f,0.0f};
    m->vertices[2] = (vertex){{ 1.0f, 1.0f, 0.0f},{0.0f,0.0f,1.0f},1.0f,1.0f};
    m->vertices[3] = (vertex){{-1.0f, 1.0f, 0.0f},{0.0f,0.0f,1.0f},0.0f,1.0f};

    m->triangles[0] = (triangle){0,1,2,0};
    m->triangles[1] = (triangle){0,2,3,0};

    out_scene->materials[0] = (material){
        .albedo = {1.0f, 1.0f, 1.0f},
        .roughness = 0.2f,
        .metallic = 0.0f,
        .albedo_texture = 0,
        .normal_texture = 1
    };

    for (size_t i = 0; i < out_scene->texture_count; ++i) {
        out_scene->textures[i].width = 256;
        out_scene->textures[i].height = 256;
        out_scene->textures[i].rgba8 = (uint8_t*)calloc(256 * 256 * 4, 1);
        if (!out_scene->textures[i].rgba8) {
            destroy_scene(out_scene);
            return 0;
        }
    }

    texture *albedo = &out_scene->textures[0];
    texture *normal = &out_scene->textures[1];

    for (uint32_t y = 0; y < albedo->height; ++y) {
        for (uint32_t x = 0; x < albedo->width; ++x) {
            float u = (float)x / (float)(albedo->width - 1);
            float v = (float)y / (float)(albedo->height - 1);
            int checker = (((x / 32) + (y / 32)) & 1);
            vec3 c = checker ? (vec3){0.8f, 0.2f, 0.2f} : (vec3){0.2f, 0.8f, 0.2f};
            size_t idx = (size_t)(y * albedo->width + x) * 4;
            albedo->rgba8[idx + 0] = clamp255(c.x * (0.5f + 0.5f * u));
            albedo->rgba8[idx + 1] = clamp255(c.y * (0.5f + 0.5f * v));
            albedo->rgba8[idx + 2] = clamp255(c.z);
            albedo->rgba8[idx + 3] = 255;

            vec3 n = vec3_norm((vec3){sinf(u*6.2831f)*0.4f, cosf(v*6.2831f)*0.4f, 1.0f});
            normal->rgba8[idx + 0] = clamp255(0.5f * (n.x + 1.0f));
            normal->rgba8[idx + 1] = clamp255(0.5f * (n.y + 1.0f));
            normal->rgba8[idx + 2] = clamp255(0.5f * (n.z + 1.0f));
            normal->rgba8[idx + 3] = 255;
        }
    }

    return 1;
}

void destroy_scene(scene *s) {
    if (!s) return;
    for (size_t i = 0; i < s->mesh_count; ++i) {
        free(s->meshes[i].vertices);
        free(s->meshes[i].triangles);
    }
    for (size_t i = 0; i < s->texture_count; ++i) {
        free(s->textures[i].rgba8);
    }
    free(s->meshes);
    free(s->textures);
    free(s->materials);
    memset(s, 0, sizeof(*s));
}
