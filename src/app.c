#include "app.h"

#include <stdio.h>
#include <stdlib.h>

#include "scene.h"
#include "software_rt.h"
#include "vulkan_rt.h"

static int write_ppm(const char *path, const framebuffer *fb) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    fprintf(f, "P6\n%u %u\n255\n", fb->width, fb->height);
    for (uint32_t i = 0; i < fb->width * fb->height; ++i) {
        fwrite(&fb->rgba8[i * 4], 1, 3, f);
    }
    fclose(f);
    return 1;
}

int run_app(void) {
    scene s;
    if (!build_demo_scene(&s)) {
        fprintf(stderr, "Failed to build scene\n");
        return 1;
    }

#ifdef ENABLE_HARDWARE_RT
    if (!render_hardware_vulkan(&s)) {
        fprintf(stderr, "Hardware path unavailable or failed, continuing with software fallback.\n");
    }
#endif

#ifdef ENABLE_SOFTWARE_RT
    framebuffer fb = {0};
    fb.width = 640;
    fb.height = 360;
    fb.rgba8 = (uint8_t*)calloc((size_t)fb.width * fb.height * 4, 1);
    if (!fb.rgba8) {
        destroy_scene(&s);
        return 1;
    }

    if (!render_software(&s, &fb)) {
        fprintf(stderr, "Software rendering failed\n");
        free(fb.rgba8);
        destroy_scene(&s);
        return 1;
    }

    if (!write_ppm("output.ppm", &fb)) {
        fprintf(stderr, "Failed to write output.ppm\n");
    } else {
        printf("Software render complete: output.ppm\n");
    }

    free(fb.rgba8);
#endif

    destroy_scene(&s);
    return 0;
}
