#include "vulkan_rt.h"

#ifdef ENABLE_HARDWARE_RT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

int render_hardware_vulkan(const scene *s) {
    (void)s;
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "vk_hybrid_raytracer",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName = "custom",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion = VK_API_VERSION_1_3
    };

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info
    };

    VkInstance instance = VK_NULL_HANDLE;
    VkResult r = vkCreateInstance(&create_info, NULL, &instance);
    if (r != VK_SUCCESS) {
        fprintf(stderr, "Failed to create Vulkan instance: %d\n", (int)r);
        return 0;
    }

    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, NULL);
    if (count == 0) {
        fprintf(stderr, "No Vulkan GPU detected.\n");
        vkDestroyInstance(instance, NULL);
        return 0;
    }

    VkPhysicalDevice *devices = (VkPhysicalDevice*)calloc(count, sizeof(VkPhysicalDevice));
    if (!devices) {
        vkDestroyInstance(instance, NULL);
        return 0;
    }
    vkEnumeratePhysicalDevices(instance, &count, devices);

    int hw_rt_supported = 0;
    for (uint32_t i = 0; i < count; ++i) {
        VkPhysicalDeviceAccelerationStructureFeaturesKHR accel = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR
        };
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtpipe = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
            .pNext = &accel
        };
        VkPhysicalDeviceFeatures2 f2 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &rtpipe
        };
        vkGetPhysicalDeviceFeatures2(devices[i], &f2);
        if (rtpipe.rayTracingPipeline && accel.accelerationStructure) {
            hw_rt_supported = 1;
            break;
        }
    }

    free(devices);
    vkDestroyInstance(instance, NULL);

    if (!hw_rt_supported) {
        fprintf(stderr, "Hardware RT extensions unavailable; use software backend.\n");
        return 0;
    }

    printf("Hardware Vulkan RT capability detected. Slang shader: shaders/raytracing.slang\n");
    return 1;
}
#else
int render_hardware_vulkan(const scene *s) {
    (void)s;
    return 0;
}
#endif
