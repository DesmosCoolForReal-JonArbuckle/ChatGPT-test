# Vulkan Hybrid Ray Tracer (C + Slang)

This project contains a cross-platform (Windows/Linux) hybrid ray tracer in C with:

- **Software ray tracing** fallback (CPU).
- **Hardware Vulkan backend with dual GPU paths**:
  - Dedicated RT pipeline path (`VK_KHR_ray_tracing_pipeline` + acceleration structures).
  - Compute fallback path for GPUs without dedicated RT pipeline support.
- **Mesh triangle rendering** with BVH-backed traversal API.
- **Materials**, **albedo textures**, and **normal map sampling** in the software path.
- **Separate Slang shader files** for dedicated RT and compute fallback.
- **Debug/Release build modes** via CMake.

## Build

### Linux

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_HARDWARE_RT=OFF
cmake --build build
./build/vk_hybrid_raytracer
```

### Windows (Visual Studio example)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
cmake --build build --config Release
```

### Hybrid mode (hardware + software)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_HARDWARE_RT=ON -DENABLE_SOFTWARE_RT=ON
cmake --build build
./build/vk_hybrid_raytracer
```

## Runtime backend behavior

When hardware mode is enabled, runtime selects:

1. **Dedicated RT pipeline backend** if required Vulkan RT extensions + features are present.
2. **Compute fallback backend** when dedicated RT is not available but compute-capable Vulkan is available.
3. Falls back to software backend if hardware init/submit fails.

## Output

- Software backend writes `output.ppm`.

## File map

- `src/vulkan_rt.c`: Vulkan device discovery, dedicated-RT vs compute-fallback selection, logical device creation, queue submit smoke test.
- `shaders/raytracing.slang`: dedicated RT shader entry points.
- `shaders/compute_fallback.slang`: compute fallback shader entry point.
- `src/software_rt.c`: CPU tracing/shading path.
- `src/bvh.c`: BVH and intersections.
