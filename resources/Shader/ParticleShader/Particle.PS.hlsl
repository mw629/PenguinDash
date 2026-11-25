#include "Particle.hlsli" 

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float32_t4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    output.color = gMaterial.color * textureColor * input.color;
    
    // テクスチャの輝度またはアルファ値を考慮して透過処理を行う
    // 黒背景画像（A=1.0）でもアルファPNGでも正しく四角が抜けるように輝度とアルファを併用
    float32_t rgbLuma = max(textureColor.r, max(textureColor.g, textureColor.b));
    float32_t mask = (textureColor.a < 0.999f) ? textureColor.a : rgbLuma;
    output.color.a *= mask;

    // UV中心からの距離による円形ソフト減衰（クアッドの四角い角・輪郭を完全にカット）
    float2 centerOffset = input.texcoord - float2(0.5f, 0.5f);
    float dist = length(centerOffset) * 2.0f;
    float circularFade = smoothstep(1.0f, 0.7f, dist);
    output.color.a *= circularFade;

    if (output.color.a <= 0.001f)
    {
        discard;
    }
   
    return output;
}