#include "PostEffect.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
Texture2D<float32_t> gMaskTexture : register(t1);
sampler gSampler : register(s0);

ConstantBuffer<PostEffectData> gPostEffect : register(b0);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    output.color = gTexture.Sample(gSampler, input.texcoord); 
    
    float32_t4 color = output.color;
    float steps = gPostEffect.value1;
    
    // steps が 2 未満（0 や 1）の場合はそのまま出力（黒画面防止）
    if (steps < 2.0)
    {
        return output;
    }
    
    // 1. 人間の目の感度に基づいた輝度（明るさ）を計算
    float lum = dot(color.rgb, float3(0.299, 0.587, 0.114));
    
    // 2. 輝度だけを段階化
    float posterizedLum = floor(lum * steps) / (steps - 1.0);
    
    // 3. 元の色の比率を保ったまま輝度を掛け直す
    output.color = color * (posterizedLum / max(lum, 0.0001));
    
    return output;
}
