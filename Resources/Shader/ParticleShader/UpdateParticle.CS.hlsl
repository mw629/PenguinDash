#include "Particle.hlsli"

RWStructuredBuffer<Particle> gParticles : register(u0);

ConstantBuffer<PerFrame> gPerFrame : register(b0);

[numthreads(1024, 1, 1)]
void main(uint32_t3 DTid : SV_DispatchThreadID)
{
    uint32_t particleIndex = DTid.x;
    if (particleIndex < kMaxParticles)
    {
        // alphaが0のparticleは生きていないとみなし更新しない
        if (gParticles[particleIndex].color.a != 0.0f)
        {
            gParticles[particleIndex].velocity += gPerFrame.acceleration;
            
            // --- Field Logic Start ---
            if (gPerFrame.fieldType == 1) // PointGravity (Attractor)
            {
                float32_t3 dir = gPerFrame.fieldPosition - gParticles[particleIndex].translate;
                float32_t dist = length(dir);
                if (dist > 0.01f) {
                    dir = normalize(dir);
                    float32_t force = gPerFrame.fieldStrength / (dist * dist);
                    gParticles[particleIndex].velocity += dir * force * gPerFrame.deltaTime;
                }
            }
            else if (gPerFrame.fieldType == 2) // Vortex
            {
                float32_t3 dir = gParticles[particleIndex].translate - gPerFrame.fieldPosition;
                dir.y = 0.0f; // Vortex around Y axis
                float32_t dist = length(dir);
                if (dist > 0.01f) {
                    float32_t3 tangent = float32_t3(-dir.z, 0.0f, dir.x); // perpendicular
                    tangent = normalize(tangent);
                    float32_t force = gPerFrame.fieldStrength / dist;
                    
                    // 遠心力で外側に飛んでいくのを防ぐため、XZ平面の速度を適度に減衰させる
                    gParticles[particleIndex].velocity.x *= 0.9f;
                    gParticles[particleIndex].velocity.z *= 0.9f;
                    
                    gParticles[particleIndex].velocity += tangent * force * gPerFrame.deltaTime;
                    // 引力も少し加える
                    gParticles[particleIndex].velocity += normalize(-dir) * (force * 0.1f) * gPerFrame.deltaTime;
                }
            }
            // --- Field Logic End ---

            gParticles[particleIndex].translate += gParticles[particleIndex].velocity;
            gParticles[particleIndex].scale *= gPerFrame.sizeDelta;
            gParticles[particleIndex].currentTime += gPerFrame.deltaTime;
            float32_t lifetime = gParticles[particleIndex].lifetime > 0.0f ? gParticles[particleIndex].lifetime : 3.0f;
            float32_t alpha = 1.0f - (gParticles[particleIndex].currentTime / lifetime);
            gParticles[particleIndex].color.a = saturate(alpha);
        }
    }  
}
