#include "Particle.hlsli"

ConstantBuffer<EmitterBox> gEmitterBox : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int32_t> gFreeCounter : register(u1);

[numthreads(1024, 1, 1)]
void main(uint32_t3 DTid : SV_DispatchThreadID)
{
    if (gEmitterBox.emit != 0 && DTid.x < (uint32_t)gEmitterBox.count)
    {
        RandomGenerator generator;
        generator.seed = (DTid.x + 1.0f) * (gPerFrame.time + 0.1f) * 1337.0f;

        int32_t particleIndex;
        InterlockedAdd(gFreeCounter[0], 1, particleIndex);
        particleIndex = particleIndex % (int32_t)kMaxParticles;

        float32_t3 randomPos = generator.Generate3d() * 2.0f - 1.0f;
        float32_t3 randomVel = generator.Generate3d() * 2.0f - 1.0f;
        float32_t3 randomScale = generator.Generate3d() * 2.0f - 1.0f;

        float32_t3 pos = gEmitterBox.translate + randomPos * (gEmitterBox.size * 0.5f);

        gParticles[particleIndex].translate = pos;
        gParticles[particleIndex].scale = gEmitterBox.baseScale + randomScale * gEmitterBox.sizeVariance;
        gParticles[particleIndex].rotate = gEmitterBox.baseRotate;
        gParticles[particleIndex].velocity = gEmitterBox.baseVelocity + randomVel * gEmitterBox.velocityVariance;
        gParticles[particleIndex].lifetime = gEmitterBox.lifetime > 0.0f ? gEmitterBox.lifetime : 3.0f;
        gParticles[particleIndex].currentTime = 0.0f;
        gParticles[particleIndex].color = gEmitterBox.color;
    }
}