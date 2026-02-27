// Compute fallback for GPUs lacking dedicated RT pipeline support.
// Traversal is expected to run against BVH buffers in compute.

struct Material {
    float3 albedo;
    float roughness;
    float metallic;
    uint albedoTex;
    uint normalTex;
};

[numthreads(8, 8, 1)]
[shader("compute")]
void computeMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    // Production fallback path:
    // 1) Generate camera ray for dispatchThreadID.xy
    // 2) Traverse BVH in compute
    // 3) Intersect triangles
    // 4) Sample textures + normal map
    // 5) Store lit color in output image
}
