# Architecture

## Backends

- `ENABLE_HARDWARE_RT`: Vulkan-based hardware RT path (feature probe and extension point).
- `ENABLE_SOFTWARE_RT`: CPU fallback path that guarantees rendering output.

## Optimization techniques implemented

- Axis-aligned bounds on geometry for broad-phase culling.
- BVH API with room for SAH/SBVH/LBVH splitting upgrades.
- Möller–Trumbore ray-triangle intersections.
- Barycentric interpolation for UVs and normals.

## Material workflow

- Base color (albedo)
- Roughness/metallic scalar factors
- Albedo texture sampling
- Normal map decoding (tangent-space approximation)

## Platform notes

- Project builds as C11.
- CMake handles Windows/Linux generation and Debug/Release variants.
