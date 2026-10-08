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
    float32_t mask = max(textureColor.a, max(textureColor.r, max(textureColor.g, textureColor.b)));
    output.color.a *= mask;

    if (output.color.a <= 0.001f)
    {
        discard;
    }
   
    return output;
}