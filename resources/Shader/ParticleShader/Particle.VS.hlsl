#include "Particle.hlsli"

StructuredBuffer<Particle> gParticle : register(t0);
ConstantBuffer<PerView> gPerView : register(b0);

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    VertexShaderOutput output;
    Particle particle = gParticle[instanceId];
    
    // 頂点のローカル座標
    float32_t3 pos = input.position.xyz;

    // スケール
    pos.x *= particle.scale.x;
    pos.y *= particle.scale.y;
    pos.z *= particle.scale.z;

    // ローカル回転
    if (particle.rotate.x != 0.0f || particle.rotate.y != 0.0f || particle.rotate.z != 0.0f)
    {
        float32_t3 rad = particle.rotate;
        float32_t3 s = sin(rad);
        float32_t3 c = cos(rad);
        float32_t3x3 rotX = float32_t3x3(1, 0, 0, 0, c.x, -s.x, 0, s.x, c.x);
        float32_t3x3 rotY = float32_t3x3(c.y, 0, s.y, 0, 1, 0, -s.y, 0, c.y);
        float32_t3x3 rotZ = float32_t3x3(c.z, -s.z, 0, s.z, c.z, 0, 0, 0, 1);
        float32_t3x3 rotMat = mul(rotZ, mul(rotX, rotY));
        pos = mul(pos, rotMat);
    }

    // ビルボード回転（カメラの向きに合わせる）
    float32_t3x3 billboard = (float32_t3x3)gPerView.billboardMatrix;
    pos = mul(pos, billboard);

    // 平行移動
    pos += particle.translate;

    // ビュープロジェクション変換
    output.position = mul(float32_t4(pos, 1.0f), gPerView.viewProjection);
    output.texcoord = input.texcoord;
    output.color = particle.color;

    return output;
}