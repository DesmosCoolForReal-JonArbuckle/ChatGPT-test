// Dedicated Vulkan ray tracing shader path (VK_KHR_ray_tracing_pipeline).

struct Material {
    float3 albedo;
    float roughness;
    float metallic;
    uint albedoTex;
    uint normalTex;
};

struct Payload {
    float3 radiance;
    float hitT;
    uint hit;
};

[shader("raygeneration")]
void raygenMain()
{
    // Production path:
    // 1) Build primary ray from camera uniforms
    // 2) traceRayEXT into TLAS
    // 3) resolve payload.radiance to UAV output
}

[shader("miss")]
void missMain(inout Payload payload)
{
    payload.radiance = float3(0.03, 0.03, 0.05);
    payload.hitT = -1.0;
    payload.hit = 0;
}

[shader("closesthit")]
void closestHitMain(inout Payload payload)
{
    // Production path:
    // interpolate barycentrics, sample albedo + normal map, run BRDF.
    payload.radiance = float3(1.0, 0.0, 1.0);
    payload.hitT = 1.0;
    payload.hit = 1;
}
