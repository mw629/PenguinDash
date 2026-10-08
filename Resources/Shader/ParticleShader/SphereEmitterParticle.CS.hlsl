#include "Particle.hlsli"

ConstantBuffer<EmitterSphere> gEmitterSphere : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int32_t> gFreeCounter : register(u1);


[numthreads(1024, 1, 1)]
void main(uint32_t3 DTid : SV_DispatchThreadID)
{
    if (gEmitterSphere.emit != 0 && DTid.x < (uint32_t)gEmitterSphere.count)
    {
        RandomGenerator generator;
        generator.seed = (DTid.x + 1.0f) * (gPerFrame.time + 0.1f) * 1337.0f;

        int32_t particleIndex;
        InterlockedAdd(gFreeCounter[0], 1, particleIndex);
        particleIndex = particleIndex % (int32_t)kMaxParticles;

        float32_t3 randomPos = generator.Generate3d() * 2.0f - 1.0f;
        float32_t3 randomVel = generator.Generate3d() * 2.0f - 1.0f;
        float32_t3 randomScale = generator.Generate3d() * 2.0f - 1.0f;

        gParticles[particleIndex].translate = gEmitterSphere.translate + randomPos * gEmitterSphere.radius;
        gParticles[particleIndex].scale = gEmitterSphere.baseScale + randomScale * gEmitterSphere.sizeVariance;
        gParticles[particleIndex].rotate = gEmitterSphere.baseRotate;
        gParticles[particleIndex].velocity = gEmitterSphere.baseVelocity + randomVel * gEmitterSphere.velocityVariance;
        gParticles[particleIndex].lifetime = gEmitterSphere.lifetime > 0.0f ? gEmitterSphere.lifetime : 3.0f;
        gParticles[particleIndex].currentTime = 0.0f;
        gParticles[particleIndex].color = gEmitterSphere.color;
    }
}