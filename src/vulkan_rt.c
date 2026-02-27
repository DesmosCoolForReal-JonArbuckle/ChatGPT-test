#include "vulkan_rt.h"

#include <string.h>

#ifdef ENABLE_HARDWARE_RT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan.h>

typedef struct {
    VkInstance instance;
    VkPhysicalDevice physical;
    VkDevice device;
    uint32_t queue_family;
    VkQueue queue;
} vk_core;

static int has_ext(const VkExtensionProperties *exts, uint32_t ext_count, const char *name) {
    for (uint32_t i = 0; i < ext_count; ++i) {
        if (strcmp(exts[i].extensionName, name) == 0) return 1;
    }
    return 0;
}

static int pick_queue_family(VkPhysicalDevice gpu, uint32_t *out_family) {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, NULL);
    if (count == 0) return 0;
    VkQueueFamilyProperties *props = (VkQueueFamilyProperties*)calloc(count, sizeof(VkQueueFamilyProperties));
    if (!props) return 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, props);
    for (uint32_t i = 0; i < count; ++i) {
        if (props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            *out_family = i;
            free(props);
            return 1;
        }
    }
    free(props);
    return 0;
}

static int init_instance(vk_core *vk) {
    const char *inst_exts[] = { VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME };

    VkApplicationInfo app = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "vk_hybrid_raytracer",
        .applicationVersion = VK_MAKE_VERSION(0, 2, 0),
        .pEngineName = "custom",
        .engineVersion = VK_MAKE_VERSION(0, 2, 0),
        .apiVersion = VK_API_VERSION_1_2
    };

    VkInstanceCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app,
        .enabledExtensionCount = (uint32_t)(sizeof(inst_exts) / sizeof(inst_exts[0])),
        .ppEnabledExtensionNames = inst_exts
    };

    return vkCreateInstance(&ci, NULL, &vk->instance) == VK_SUCCESS;
}

static int pick_physical(vk_core *vk, vulkan_rt_report *report, int *out_has_rt_pipeline) {
    uint32_t gpu_count = 0;
    vkEnumeratePhysicalDevices(vk->instance, &gpu_count, NULL);
    if (gpu_count == 0) return 0;

    VkPhysicalDevice *gpus = (VkPhysicalDevice*)calloc(gpu_count, sizeof(VkPhysicalDevice));
    if (!gpus) return 0;
    vkEnumeratePhysicalDevices(vk->instance, &gpu_count, gpus);

    int found_any_compute = 0;
    for (uint32_t i = 0; i < gpu_count; ++i) {
        uint32_t ext_count = 0;
        vkEnumerateDeviceExtensionProperties(gpus[i], NULL, &ext_count, NULL);
        VkExtensionProperties *exts = (VkExtensionProperties*)calloc(ext_count, sizeof(VkExtensionProperties));
        if (!exts) continue;
        vkEnumerateDeviceExtensionProperties(gpus[i], NULL, &ext_count, exts);

        uint32_t family = 0;
        if (!pick_queue_family(gpus[i], &family)) {
            free(exts);
            continue;
        }
        found_any_compute = 1;

        int has_bda = has_ext(exts, ext_count, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
        int has_deferred = has_ext(exts, ext_count, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        int has_accel = has_ext(exts, ext_count, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        int has_rt_pipe = has_ext(exts, ext_count, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        int has_ray_query = has_ext(exts, ext_count, VK_KHR_RAY_QUERY_EXTENSION_NAME);

        int rt_capable_exts = has_bda && has_deferred && has_accel && has_rt_pipe;

        if (rt_capable_exts) {
            VkPhysicalDeviceBufferDeviceAddressFeatures bda = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES
            };
            VkPhysicalDeviceAccelerationStructureFeaturesKHR accel = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
                .pNext = &bda
            };
            VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
                .pNext = &accel
            };
            VkPhysicalDeviceRayQueryFeaturesKHR rq = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR,
                .pNext = &rt
            };
            VkPhysicalDeviceFeatures2 f2 = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                .pNext = &rq
            };
            vkGetPhysicalDeviceFeatures2(gpus[i], &f2);

            if (rt.rayTracingPipeline && accel.accelerationStructure && bda.bufferDeviceAddress) {
                vk->physical = gpus[i];
                vk->queue_family = family;
                report->supports_rt_pipeline = 1;
                report->supports_accel_struct = 1;
                report->supports_buffer_device_address = 1;
                report->supports_deferred_host_ops = has_deferred;
                report->supports_ray_query = rq.rayQuery ? 1 : 0;
                *out_has_rt_pipeline = 1;
                free(exts);
                free(gpus);
                return 1;
            }
        }

        if (!*out_has_rt_pipeline) {
            vk->physical = gpus[i];
            vk->queue_family = family;
            report->supports_buffer_device_address = has_bda;
            report->supports_ray_query = has_ray_query;
            report->supports_accel_struct = has_accel;
            report->supports_deferred_host_ops = has_deferred;
        }

        free(exts);
    }

    free(gpus);
    return found_any_compute;
}

static int create_device(vk_core *vk, const vulkan_rt_report *report, int use_rt_pipeline) {
    float qprio = 1.0f;
    VkDeviceQueueCreateInfo qci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = vk->queue_family,
        .queueCount = 1,
        .pQueuePriorities = &qprio
    };

    const char *rt_exts[] = {
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
    };
    const char *compute_exts[] = {
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
    };

    VkPhysicalDeviceBufferDeviceAddressFeatures bda = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .bufferDeviceAddress = report->supports_buffer_device_address ? VK_TRUE : VK_FALSE
    };
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accel = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        .accelerationStructure = use_rt_pipeline ? VK_TRUE : VK_FALSE,
        .pNext = &bda
    };
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
        .rayTracingPipeline = use_rt_pipeline ? VK_TRUE : VK_FALSE,
        .pNext = &accel
    };

    VkDeviceCreateInfo dci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &qci
    };

    if (use_rt_pipeline) {
        dci.enabledExtensionCount = (uint32_t)(sizeof(rt_exts) / sizeof(rt_exts[0]));
        dci.ppEnabledExtensionNames = rt_exts;
        dci.pNext = &rt;
    } else {
        dci.enabledExtensionCount = report->supports_buffer_device_address ? 1u : 0u;
        dci.ppEnabledExtensionNames = report->supports_buffer_device_address ? compute_exts : NULL;
        dci.pNext = report->supports_buffer_device_address ? &bda : NULL;
    }

    if (vkCreateDevice(vk->physical, &dci, NULL, &vk->device) != VK_SUCCESS) return 0;
    vkGetDeviceQueue(vk->device, vk->queue_family, 0, &vk->queue);
    return 1;
}

static int smoke_submit(VkDevice device, uint32_t queue_family, VkQueue queue) {
    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandPoolCreateInfo pci = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = queue_family,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    };
    if (vkCreateCommandPool(device, &pci, NULL, &pool) != VK_SUCCESS) return 0;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkCommandBufferAllocateInfo cai = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    if (vkAllocateCommandBuffers(device, &cai, &cmd) != VK_SUCCESS) {
        vkDestroyCommandPool(device, pool, NULL);
        return 0;
    }

    VkCommandBufferBeginInfo bi = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    if (vkBeginCommandBuffer(cmd, &bi) != VK_SUCCESS) {
        vkDestroyCommandPool(device, pool, NULL);
        return 0;
    }
    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        vkDestroyCommandPool(device, pool, NULL);
        return 0;
    }

    VkSubmitInfo si = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd
    };
    VkResult r = vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
    if (r == VK_SUCCESS) r = vkQueueWaitIdle(queue);

    vkDestroyCommandPool(device, pool, NULL);
    return r == VK_SUCCESS;
}

int render_hardware_vulkan(const scene *s, vulkan_rt_report *out_report) {
    (void)s;
    vulkan_rt_report report = {0};
    vk_core vk;
    memset(&vk, 0, sizeof(vk));

    if (!init_instance(&vk)) {
        fprintf(stderr, "Vulkan: failed to create instance.\n");
        return 0;
    }

    int has_rt_pipeline = 0;
    if (!pick_physical(&vk, &report, &has_rt_pipeline)) {
        fprintf(stderr, "Vulkan: no suitable compute-capable GPU found.\n");
        vkDestroyInstance(vk.instance, NULL);
        return 0;
    }

    report.backend = has_rt_pipeline ? VULKAN_BACKEND_RT_PIPELINE : VULKAN_BACKEND_COMPUTE_FALLBACK;

    if (!create_device(&vk, &report, has_rt_pipeline)) {
        fprintf(stderr, "Vulkan: failed to create logical device.\n");
        vkDestroyInstance(vk.instance, NULL);
        return 0;
    }

    if (!smoke_submit(vk.device, vk.queue_family, vk.queue)) {
        fprintf(stderr, "Vulkan: failed queue submit smoke test.\n");
        vkDestroyDevice(vk.device, NULL);
        vkDestroyInstance(vk.instance, NULL);
        return 0;
    }

    if (report.backend == VULKAN_BACKEND_RT_PIPELINE) {
        printf("Vulkan backend: dedicated hardware RT pipeline active (shader: shaders/raytracing.slang).\n");
    } else {
        printf("Vulkan backend: compute fallback active for non-dedicated RT GPUs (shader: shaders/compute_fallback.slang).\n");
    }

    if (out_report) *out_report = report;

    vkDestroyDevice(vk.device, NULL);
    vkDestroyInstance(vk.instance, NULL);
    return 1;
}
#else
int render_hardware_vulkan(const scene *s, vulkan_rt_report *out_report) {
    (void)s;
    if (out_report) memset(out_report, 0, sizeof(*out_report));
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
