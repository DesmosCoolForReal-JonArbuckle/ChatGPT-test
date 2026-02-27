#ifndef VULKAN_RT_H
#define VULKAN_RT_H

#include "scene.h"

typedef enum {
    VULKAN_BACKEND_NONE = 0,
    VULKAN_BACKEND_RT_PIPELINE = 1,
    VULKAN_BACKEND_COMPUTE_FALLBACK = 2
} vulkan_backend_kind;

typedef struct {
    vulkan_backend_kind backend;
    int supports_rt_pipeline;
    int supports_accel_struct;
    int supports_ray_query;
    int supports_buffer_device_address;
    int supports_deferred_host_ops;
} vulkan_rt_report;

int render_hardware_vulkan(const scene *s, vulkan_rt_report *out_report);

#endif
