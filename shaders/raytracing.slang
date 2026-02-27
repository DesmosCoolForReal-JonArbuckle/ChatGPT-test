// Slang shader skeleton for Vulkan ray tracing pipeline.
struct Payload {
    float3 radiance;
    uint hit;
};

[shader("raygeneration")]
void raygenMain() {
    // Integrate TLAS traversal, material sampling and texture/normal maps here.
}

[shader("miss")]
void missMain(inout Payload payload) {
    payload.radiance = float3(0.03, 0.03, 0.05);
    payload.hit = 0;
}

[shader("closesthit")]
void closestHitMain(inout Payload payload) {
    payload.radiance = float3(1.0, 0.0, 1.0);
    payload.hit = 1;
}
