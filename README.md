# Vulkan Hybrid Ray Tracer (C + Slang)

This repository now contains a cross-platform (Windows/Linux) hybrid ray tracer in C with:

- **Software ray tracing** fallback (CPU).
- **Hardware ray tracing capability check** for Vulkan RT (acceleration structure + RT pipeline features).
- **Mesh triangle rendering** with a BVH-style acceleration layer.
- **Materials**, **albedo textures**, and **normal map sampling** in the software path.
- **Slang shader source** scaffold for Vulkan ray tracing stages.
- **Debug/Release build modes** via CMake.

> Note: The Vulkan backend includes capability and initialization plumbing plus Slang shader integration points. A production-ready full Vulkan RT pipeline setup (SBT, descriptor sets, AS build, etc.) is substantial and usually split into additional modules.

## Build

### Linux

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_HARDWARE_RT=OFF
cmake --build build
./build/vk_hybrid_raytracer
```

### Windows (Visual Studio generator example)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
cmake --build build --config Release
```

### Hardware + software mode

If Vulkan SDK is installed:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_HARDWARE_RT=ON -DENABLE_SOFTWARE_RT=ON
cmake --build build
```

## Output

The software backend writes a sample render to:

- `output.ppm`

## File map

- `src/software_rt.c`: CPU path tracer/raster-like direct-light implementation over triangle meshes.
- `src/bvh.c`: BVH container and triangle intersection traversal API.
- `src/vulkan_rt.c`: Vulkan device/RT capability probing and backend entrypoint.
- `shaders/raytracing.slang`: Slang ray tracing shader entry-point skeleton.
