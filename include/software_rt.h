#ifndef SOFTWARE_RT_H
#define SOFTWARE_RT_H

#include <stdint.h>
#include "scene.h"

typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t *rgba8;
} framebuffer;

int render_software(const scene *s, framebuffer *fb);

#endif
