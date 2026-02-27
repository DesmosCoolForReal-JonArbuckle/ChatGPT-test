#ifndef SCENE_H
#define SCENE_H

#include <stddef.h>
#include <stdint.h>
#include "math3d.h"

typedef struct {
    vec3 albedo;
    float roughness;
    float metallic;
    int albedo_texture;
    int normal_texture;
} material;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t *rgba8;
} texture;

typedef struct {
    vec3 position;
    vec3 normal;
    float u;
    float v;
} vertex;

typedef struct {
    uint32_t i0;
    uint32_t i1;
    uint32_t i2;
    int material_index;
} triangle;

typedef struct {
    vertex *vertices;
    size_t vertex_count;
    triangle *triangles;
    size_t triangle_count;
} mesh;

typedef struct {
    mesh *meshes;
    size_t mesh_count;
    texture *textures;
    size_t texture_count;
    material *materials;
    size_t material_count;
} scene;

int build_demo_scene(scene *out_scene);
void destroy_scene(scene *s);
vec3 sample_texture(const texture *tx, float u, float v);

#endif
