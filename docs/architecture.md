# Architecture

## Backends

- `ENABLE_HARDWARE_RT`: Vulkan backend with two runtime-selected modes:
  - **Dedicated RT pipeline**: requires `VK_KHR_ray_tracing_pipeline` + `VK_KHR_acceleration_structure` + related dependencies.
  - **Compute fallback**: selected when dedicated RT is unavailable; intended to run BVH traversal/intersections in compute shaders.
- `ENABLE_SOFTWARE_RT`: CPU fallback path that always produces output.

## Vulkan mode selection

At runtime (`src/vulkan_rt.c`):

1. Create Vulkan instance.
2. Enumerate devices and queue families.
3. Probe extensions/features for dedicated RT.
4. If dedicated RT features are present, create a dedicated RT-capable logical device.
5. Else create a compute-capable logical device and select compute fallback shader path.
6. Submit command-buffer smoke test to validate queue/device health.

## Optimization techniques

- AABB broad-phase culling.
- BVH traversal API with extension room for SAH/SBVH/LBVH splits.
- Möller–Trumbore ray/triangle test.
- Barycentric UV/normal interpolation.

## Material workflow

- Base color (albedo)
- Roughness/metallic scalar factors
- Albedo texture sampling
- Normal map decoding

## Shader layout

- `shaders/raytracing.slang`: dedicated RT (`raygeneration`, `miss`, `closesthit`).
- `shaders/compute_fallback.slang`: compute fallback (`compute`).
